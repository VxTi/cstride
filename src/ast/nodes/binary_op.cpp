#include "ast/nodes/binary_op.h"
#include <llvm/IR/IRBuilder.h>
#include <format>

using namespace stride::ast;

AstBinaryOp::AstBinaryOp(
    std::unique_ptr<AstExpression> left,
    TokenType op,
    std::unique_ptr<AstExpression> right
) :
    AstExpression({}),
    left(std::move(left)),
    op(op),
    right(std::move(right)) {}

std::string AstBinaryOp::to_string()
{
    return std::format(
        "BinaryOp({}, {}, {})",
        left->to_string(),
        token_type_to_str(op),
        right->to_string()
    );
}

llvm::Value* AstBinaryOp::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irbuilder)
{
    llvm::Value* l = left->codegen(module, context, irbuilder);
    llvm::Value* r = right->codegen(module, context, irbuilder);

    if (!l || !r)
    {
        return nullptr;
    }

    llvm::IRBuilder<> builder(context);

    // Attempt to locate insertion point
    if (auto* inst = llvm::dyn_cast<llvm::Instruction>(l))
    {
        builder.SetInsertPoint(inst->getParent());
    }
    else if (auto* inst = llvm::dyn_cast<llvm::Instruction>(r))
    {
        builder.SetInsertPoint(inst->getParent());
    }

    bool is_float = l->getType()->isFloatingPointTy();

    switch (op)
    {
    case TokenType::PLUS:
        return is_float ? builder.CreateFAdd(l, r, "addtmp") : builder.CreateAdd(l, r, "addtmp");
    case TokenType::MINUS:
        return is_float ? builder.CreateFSub(l, r, "subtmp") : builder.CreateSub(l, r, "subtmp");
    case TokenType::STAR:
        return is_float ? builder.CreateFMul(l, r, "multmp") : builder.CreateMul(l, r, "multmp");
    case TokenType::SLASH:
        return is_float ? builder.CreateFDiv(l, r, "divtmp") : builder.CreateSDiv(l, r, "divtmp");
    default:
        return nullptr;
    }
}