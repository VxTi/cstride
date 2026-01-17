#include "ast/nodes/expression.h"
#include <llvm/IR/IRBuilder.h>
#include <format>

#include "ast/nodes/literal_values.h"

using namespace stride::ast;

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
    // A binary operation is reducible if either of both sides is reducible
    // A literal doesn't extend `IReducible`, though is by nature "reducible" in followup steps.
    return (is_ast_literal(this->left.get()) && is_ast_literal(this->right.get()))
        || this->left->is_reducible()
        || this->right->is_reducible();
}

std::unique_ptr<IAstNode> reduce_literal_binary_op(
    const AstBinaryOp* self,
    AstLiteral* left_lit,
    AstLiteral* right_lit
)
{
    switch (self->op)
    {
    case TokenType::PLUS:
        if (left_lit->type() == LiteralType::INTEGER && right_lit->type() == LiteralType::INTEGER)
        {
            return std::make_unique<AstLiteral>(LiteralType::INTEGER, left_lit->value() + right_lit->value());
        }

        if (left_lit->type() == LiteralType::FLOAT && right_lit->type() == LiteralType::FLOAT)
        {
            return std::make_unique<AstLiteral>(LiteralType::FLOAT, left_lit->value() + right_lit->value());
        }
        break;
    case TokenType::MINUS:
        break;
    case TokenType::STAR:
        break;
    case TokenType::SLASH:
        break;
    default:
        return nullptr;
    }

    return nullptr;
}

std::optional<std::unique_ptr<IAstNode>> try_reduce_additive_op(
    AstBinaryOp* self,
    AstExpression* left_lit,
    AstExpression* right_lit
)
{
    if (const auto lhs_ptr_i = dynamic_cast<AstIntegerLiteral*>(left_lit);
        lhs_ptr_i != nullptr
    )
    {
        // For integers, we do allow addition of other numeric types, though
        // this will change the resulting type. E.g., adding a float to an int
        // will result in a float (32/64 bit)

        auto lval = lval_ptr->value();

        if (right_lit->type() == LiteralType::FLOAT) {}
    }

    return std::nullopt;
}

IAstNode* AstBinaryOp::reduce()
{
    if (!this->is_reducible()) return this;


    if (const auto lhs_ptr_i = dynamic_cast<AstIntegerLiteral*>(this->left.get());
       lhs_ptr_i != nullptr
   )
    {
        // For integers, we do allow addition of other numeric types, though
        // this will change the resulting type. E.g., adding a float to an int
        // will result in a float (32/64 bit)

        auto lval = lval_ptr->value();

        if (right_lit->type() == LiteralType::FLOAT) {}
    }

    return std::nullopt;

    return this;
}
