#include "ast/nodes/expression.h"

#include <format>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::string AstLogicalOp::to_string()
{
    return std::format(
        "LogicalOp({}, {}, {})",
        this->get_left()->to_string(),
        token_type_to_str(
            this->get_op_type() == LogicalOpType::AND
            ? TokenType::DOUBLE_AMPERSAND
            : TokenType::DOUBLE_PIPE),
        this->get_right()->to_string()
    );
}

std::optional<LogicalOpType> stride::ast::get_logical_op_type(
    const TokenType type
)
{
    switch (type)
    {
    case TokenType::DOUBLE_AMPERSAND:
        return LogicalOpType::AND;
    case TokenType::DOUBLE_PIPE:
        return LogicalOpType::OR;
    default:
        return std::nullopt;
    }
}

llvm::Value* AstLogicalOp::codegen(
    llvm::Module* module,
    llvm::IRBuilder<>* ir_builder
)
{
    // Implementation following short-circuiting logic
    llvm::Value* lhs_value = this->get_left()->codegen(
        module,
        ir_builder
    );

    if (!lhs_value)
        return nullptr;

    // We need to ensure we are operating on booleans (i1).
    // If language supports truthy values, we should convert here.
    // For now, assuming strict bool or implicit conversion handling elsewhere
    // but just in case, we can compare to 0 if it's an integer/pointer?
    // Let's assume for now L is i1 or we cast it.
    // Ideally we should check type.

    // Helper to convert to bool (i1)
    auto to_bool = [&](llvm::Value* val) -> llvm::Value*
    {
        if (val->getType()->isIntegerTy(1))
        {
            // Is already bool
            return val;
        }

        if (val->getType()->isIntegerTy())
        {
            return ir_builder->CreateICmpNE(
                val,
                llvm::ConstantInt::get(
                    val->getType(),
                    0),
                "to_bool"
            );
        }

        if (val->getType()->isFloatingPointTy())
        {
            return ir_builder->CreateFCmpUNE(
                val,
                llvm::ConstantFP::get(val->getType(), 0.0),
                "to_bool");
        }
        // Pointers etc can be added if needed
        return val; // fallback
    };

    lhs_value = to_bool(lhs_value);

    llvm::Function* function = ir_builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* start_bb = ir_builder->GetInsertBlock();
    llvm::BasicBlock* eval_right_bb =
        llvm::BasicBlock::Create(module->getContext(), "eval_right", function);
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(
        module->getContext(),
        "merge"
    );

    switch (this->get_op_type())
    {
    case LogicalOpType::AND:
        // AND: If l is true, eval right. If l is false, jump to merge (result false).
        ir_builder->CreateCondBr(lhs_value, eval_right_bb, merge_bb);
        break;
    case LogicalOpType::OR:
        ir_builder->CreateCondBr(lhs_value, merge_bb, eval_right_bb);
        break;
    default:
        return nullptr;
    }

    // Emit Right block
    ir_builder->SetInsertPoint(eval_right_bb);
    llvm::Value* r = this->get_right()->codegen(module, ir_builder);
    if (!r)
    {
        return nullptr;
    }

    r = to_bool(r);

    ir_builder->CreateBr(merge_bb);
    // Refresh eval_right_bb as codegen might have changed the current block
    eval_right_bb = ir_builder->GetInsertBlock();

    // Emit Merge block
    function->insert(function->end(), merge_bb);
    ir_builder->SetInsertPoint(merge_bb);
    llvm::PHINode* phi = ir_builder->CreatePHI(
        llvm::Type::getInt1Ty(module->getContext()),
        2,
        "logical_result"
    );

    if (this->get_op_type() == LogicalOpType::AND)
    {
        // Came from start_bb (L was false) -> result false
        phi->addIncoming(llvm::ConstantInt::getFalse(module->getContext()), start_bb);
        // Came from eval_right_bb (L was true, so result is R)
        phi->addIncoming(r, eval_right_bb);
    }
    else
    {
        // Came from start_bb (L was true) -> result true
        phi->addIncoming(llvm::ConstantInt::getTrue(module->getContext()), start_bb);
        // Came from eval_right_bb (L was false, so result is R)
        phi->addIncoming(r, eval_right_bb);
    }

    return phi;
}
