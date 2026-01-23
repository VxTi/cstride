#include <format>
#include <llvm/IR/IRBuilder.h>

#include "ast/nodes/expression.h"

using namespace stride::ast;

std::string AstLogicalOp::to_string()
{
    return std::format(
        "LogicalOp({}, {}, {})",
        this->get_left().to_string(),
        token_type_to_str(
            this->get_op_type() == LogicalOpType::AND
                ? TokenType::DOUBLE_AMPERSAND
                : TokenType::DOUBLE_PIPE),
        this->get_right().to_string()
    );
}

std::optional<LogicalOpType> stride::ast::get_logical_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::DOUBLE_AMPERSAND: return LogicalOpType::AND;
    case TokenType::DOUBLE_PIPE: return LogicalOpType::OR;
    default: return std::nullopt;
    }
}

llvm::Value* AstLogicalOp::codegen(const std::shared_ptr<Scope>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irbuilder)
{
    // Implementation following short-circuiting logic
    llvm::Value* lhValue = this->get_left().codegen(scope, module, context, irbuilder);

    if (!lhValue) return nullptr;

    // We need to ensure we are operating on booleans (i1).
    // If language supports truthy values, we should convert here.
    // For now, assuming strict bool or implicit conversion handling elsewhere
    // but just in case, we can compare to 0 if it's an integer/pointer?
    // Let's assume for now L is i1 or we cast it.
    // Ideally we should check type.

    // Helper to convert to bool (i1)
    auto to_bool = [&](llvm::Value* v) -> llvm::Value*
    {
        if (v->getType()->isIntegerTy(1)) return v; // Is already bool

        if (v->getType()->isIntegerTy())
        {
            return irbuilder->CreateICmpNE(v, llvm::ConstantInt::get(v->getType(), 0), "to_bool");
        }

        if (v->getType()->isFloatingPointTy())
        {
            return irbuilder->CreateFCmpUNE(v, llvm::ConstantFP::get(v->getType(), 0.0), "to_bool");
        }
        // Pointers etc can be added if needed
        return v; // fallback
    };

    lhValue = to_bool(lhValue);

    llvm::Function* function = irbuilder->GetInsertBlock()->getParent();

    llvm::BasicBlock* start_bb = irbuilder->GetInsertBlock();
    llvm::BasicBlock* eval_right_bb = llvm::BasicBlock::Create(context, "eval_right", function);
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context, "merge");

    switch (this->get_op_type())
    {
    case LogicalOpType::AND:
        // AND: If l is true, eval right. If l is false, jump to merge (result false).
        irbuilder->CreateCondBr(lhValue, eval_right_bb, merge_bb);
        break;
    case LogicalOpType::OR:
        irbuilder->CreateCondBr(lhValue, merge_bb, eval_right_bb);
        break;
    default:
        return nullptr;
    }

    // Emit Right block
    irbuilder->SetInsertPoint(eval_right_bb);
    llvm::Value* r = this->get_right().codegen(scope, module, context, irbuilder);
    if (!r) return nullptr;

    r = to_bool(r);

    irbuilder->CreateBr(merge_bb);
    // Refresh eval_right_bb as codegen might have changed the current block
    eval_right_bb = irbuilder->GetInsertBlock();

    // Emit Merge block
    function->insert(function->end(), merge_bb);
    irbuilder->SetInsertPoint(merge_bb);
    llvm::PHINode* phi = irbuilder->CreatePHI(llvm::Type::getInt1Ty(context), 2, "logical_result");

    if (this->get_op_type() == LogicalOpType::AND)
    {
        // Came from start_bb (L was false) -> result false
        phi->addIncoming(llvm::ConstantInt::getFalse(context), start_bb);
        // Came from eval_right_bb (L was true, so result is R)
        phi->addIncoming(r, eval_right_bb);
    }
    else
    {
        // Came from start_bb (L was true) -> result true
        phi->addIncoming(llvm::ConstantInt::getTrue(context), start_bb);
        // Came from eval_right_bb (L was false, so result is R)
        phi->addIncoming(r, eval_right_bb);
    }

    return phi;
}
