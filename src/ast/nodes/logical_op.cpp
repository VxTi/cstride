#include "ast/nodes/logical_op.h"
#include <llvm/IR/IRBuilder.h>
#include <format>

using namespace stride::ast;

AstLogicalOp::AstLogicalOp(
    std::unique_ptr<AstExpression> left,
    TokenType op,
    std::unique_ptr<AstExpression> right
) :
    AstExpression({}),
    left(std::move(left)),
    op(op),
    right(std::move(right)) {}

std::string AstLogicalOp::to_string()
{
    return std::format(
        "LogicalOp({}, {}, {})",
        left->to_string(),
        token_type_to_str(op),
        right->to_string()
    );
}

llvm::Value* AstLogicalOp::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irbuilder)
{
    // Implementation following short-circuiting logic
    llvm::Value* lhValue = left->codegen(module, context, irbuilder);

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
        if (v->getType()->isIntegerTy(1)) return v;
        if (v->getType()->isIntegerTy())
            return irbuilder->CreateICmpNE(v, llvm::ConstantInt::get(v->getType(), 0), "to_bool");
        if (v->getType()->isFloatingPointTy())
            return irbuilder->CreateFCmpUNE(v, llvm::ConstantFP::get(v->getType(), 0.0), "to_bool");
        // Pointers etc can be added if needed
        return v; // fallback
    };

    lhValue = to_bool(lhValue);

    llvm::Function* function = irbuilder->GetInsertBlock()->getParent();

    llvm::BasicBlock* start_bb = irbuilder->GetInsertBlock();
    llvm::BasicBlock* eval_right_bb = llvm::BasicBlock::Create(context, "eval_right", function);
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context, "merge");

    switch (op)
    {
    case TokenType::DOUBLE_AMPERSAND:
        // AND: If l is true, eval right. If l is false, jump to merge (result false).
        irbuilder->CreateCondBr(lhValue, eval_right_bb, merge_bb);
        break;
    case TokenType::DOUBLE_PIPE:
        irbuilder->CreateCondBr(lhValue, merge_bb, eval_right_bb);
        break;
    case TokenType::NOT_EQUALS:
        irbuilder->CreateCondBr(lhValue, eval_right_bb, merge_bb);
        break;
    case TokenType::LEQUALS:
        irbuilder->CreateCondBr(lhValue, merge_bb, eval_right_bb);
        break;
    case TokenType::GEQUALS:
        irbuilder->CreateCondBr(lhValue, merge_bb, eval_right_bb);
        break;

    default:
        return nullptr;
    }

    // Emit Right block
    irbuilder->SetInsertPoint(eval_right_bb);
    llvm::Value* r = right->codegen(module, context, irbuilder);
    if (!r) return nullptr;

    r = to_bool(r);

    irbuilder->CreateBr(merge_bb);
    // Refresh eval_right_bb as codegen might have changed the current block
    eval_right_bb = irbuilder->GetInsertBlock();

    // Emit Merge block
    function->insert(function->end(), merge_bb);
    irbuilder->SetInsertPoint(merge_bb);
    llvm::PHINode* phi = irbuilder->CreatePHI(llvm::Type::getInt1Ty(context), 2, "logical_result");

    if (op == TokenType::DOUBLE_AMPERSAND)
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
