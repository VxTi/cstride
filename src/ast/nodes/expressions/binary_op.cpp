#include "ast/nodes/expression.h"
#include <llvm/IR/IRBuilder.h>
#include <format>

#include "ast/nodes/literal_values.h"

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

    llvm::Instruction* instruction;

    // Attempt to locate insertion point
    if ((instruction = llvm::dyn_cast<llvm::Instruction>(l)))
    {
        builder.SetInsertPoint(instruction->getParent());
    }
    else if ((instruction = llvm::dyn_cast<llvm::Instruction>(r)))
    {
        builder.SetInsertPoint(instruction->getParent());
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

bool AstBinaryOp::is_reducible()
{
    return is_ast_literal(this->left.get()) && is_ast_literal(this->right.get());
}

std::optional<std::unique_ptr<IAstNode>> try_reduce_additive_op(
    AstBinaryOp* self,
    AstExpression* left_lit,
    AstExpression* right_lit
)
{


    if (left_lit->type() == LiteralType::INTEGER)
    {
        // For integers, we do allow addition of other numeric types, though
        // this will change the resulting type. E.g., adding a float to an int
        // will result in a float (32/64 bit)

        auto lval_ptr = dynamic_cast<AstIntegerLiteral*>(left_lit);
        if (!lval_ptr) return std::nullopt;

        auto lval = lval_ptr->value();

        if (right_lit->type() == LiteralType::FLOAT)
        {

        }
    }

    return std::nullopt;
}

IAstNode* AstBinaryOp::reduce()
{
    if (!this->is_reducible()) return this;

    // TODO: Implement
    return this;
}
