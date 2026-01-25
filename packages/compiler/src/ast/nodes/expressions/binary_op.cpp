#include <format>
#include <llvm/IR/IRBuilder.h>

#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<BinaryOpType> stride::ast::get_binary_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::STAR: return BinaryOpType::MULTIPLY;
    case TokenType::PLUS: return BinaryOpType::ADD;
    case TokenType::MINUS: return BinaryOpType::SUBTRACT;
    case TokenType::SLASH: return BinaryOpType::DIVIDE;
    default: return std::nullopt;
    }
}

int stride::ast::get_binary_operator_precedence(const BinaryOpType type)
{
    switch (type)
    {
    case BinaryOpType::POWER: return 3;
    case BinaryOpType::MULTIPLY:
    case BinaryOpType::DIVIDE:
        return 2;
    case BinaryOpType::ADD:
    case BinaryOpType::SUBTRACT:
        return 1;
    default:
        return 0;
    }
}

std::string binary_op_to_str(const BinaryOpType op)
{
    switch (op)
    {
    case BinaryOpType::ADD: return "+";
    case BinaryOpType::SUBTRACT: return "-";
    case BinaryOpType::MULTIPLY: return "*";
    case BinaryOpType::DIVIDE: return "/";
    default: return "";
    }
}

std::string AstBinaryArithmeticOp::to_string()
{
    return std::format(
        "BinaryOp({}, {}, {})",
        this->get_left().to_string(),
        binary_op_to_str(this->get_op_type()),
        this->get_right().to_string()
    );
}

/**
 * This parses expressions that requires precedence, such as binary expressions.
 * These are binary expressions, e.g., 1 + 1, 1 - 1, 1 * 1, 1 / 1, 1 % 1
 */
std::optional<std::unique_ptr<AstExpression>> stride::ast::parse_arithmetic_binary_op(
    const std::shared_ptr<Scope>& scope,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs,
    const int min_precedence
)
{
    while (true)
    {
        const auto reference_token = set.peak_next();
        // First, we'll check if the next token is a binary operator
        if (auto binary_op = get_binary_op_type(reference_token.type); binary_op.has_value())
        {
            const auto op = binary_op.value();

            const int precedence = get_binary_operator_precedence(op);

            // If the precedence is lower than the minimum required, we return the lhs
            if (precedence < min_precedence)
            {
                return lhs;
            }

            set.next();

            // If we're unable to parse the next expression part, for whatever reason,
            // we'll return nullopt. This will indicate that the expression is incomplete or invalid.
            auto rhs = parse_standalone_expression_part(scope, set);
            if (!rhs)
            {
                return std::nullopt;
            }

            // If the followup token is also a binary expression,
            // we can try to parse it with higher precedence
            if (const auto next_op = get_binary_op_type(set.peak_next_type()); next_op.has_value())
            {
                if (const int next_precedence = get_binary_operator_precedence(next_op.value());
                    precedence < next_precedence)
                {
                    if (auto rhs_opt = parse_arithmetic_binary_op(scope, set, std::move(rhs), precedence + 1); rhs_opt.
                        has_value())
                    {
                        rhs = std::move(rhs_opt.value());
                    }
                    else
                    {
                        // We're unable to parse the next expression part, which is required for a binary operation.
                        return std::nullopt;
                    }
                }
            }

            lhs = std::make_unique<AstBinaryArithmeticOp>(
                set.source(),
                reference_token.offset,
                scope,
                std::move(lhs),
                binary_op.value(),
                std::move(rhs)
            );
        }
        else
        {
            return lhs;
        }
    }
}

llvm::Value* AstBinaryArithmeticOp::codegen(
    const std::shared_ptr<Scope>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* irbuilder
)
{
    llvm::Value* l = this->get_left().codegen(scope, module, context, irbuilder);
    llvm::Value* r = this->get_right().codegen(scope, module, context, irbuilder);

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

    // Handle integer promotion for mismatched types
    if (l->getType()->isIntegerTy() && r->getType()->isIntegerTy())
    {
        const auto l_width = l->getType()->getIntegerBitWidth();
        const auto r_width = r->getType()->getIntegerBitWidth();

        if (l_width < r_width)
        {
            l = builder.CreateIntCast(l, r->getType(), true, "binop_sext");
        }
        else if (r_width < l_width)
        {
            r = builder.CreateIntCast(r, l->getType(), true, "binop_sext");
        }
    }

    bool is_float = l->getType()->isFloatingPointTy();

    switch (this->get_op_type())
    {
    case BinaryOpType::ADD:
        return is_float ? builder.CreateFAdd(l, r, "addtmp") : builder.CreateAdd(l, r, "addtmp");
    case BinaryOpType::SUBTRACT:
        return is_float ? builder.CreateFSub(l, r, "subtmp") : builder.CreateSub(l, r, "subtmp");
    case BinaryOpType::MULTIPLY:
        return is_float ? builder.CreateFMul(l, r, "multmp") : builder.CreateMul(l, r, "multmp");
    case BinaryOpType::DIVIDE:
        return is_float ? builder.CreateFDiv(l, r, "divtmp") : builder.CreateSDiv(l, r, "divtmp");
    default:
        return nullptr;
    }
}

bool AstBinaryArithmeticOp::is_reducible()
{
    // A binary operation is reducible if either of both sides is reducible
    // A literal doesn't extend `IReducible`, though is by nature "reducible" in followup steps.
    return (is_literal_ast_node(&this->get_left()) && is_literal_ast_node(&this->get_right()))
        || this->get_left().is_reducible()
        || this->get_right().is_reducible();
}

std::unique_ptr<IAstNode> reduce_literal_binary_op(
    const AstBinaryArithmeticOp* self,
    AstLiteral* left_lit,
    AstLiteral* right_lit
)
{
    switch (self->get_op_type())
    {
    case BinaryOpType::ADD:
        /*if (left_lit->type() == LiteralType::INTEGER && right_lit->type() == LiteralType::INTEGER)
        {
            return std::make_unique<AstLiteral>(LiteralType::INTEGER, left_lit->value() + right_lit->value());
        }

        if (left_lit->type() == LiteralType::FLOAT && right_lit->type() == LiteralType::FLOAT)
        {
            return std::make_unique<AstLiteral>(LiteralType::FLOAT, left_lit->value() + right_lit->value());
        }*/
        break;
    case BinaryOpType::SUBTRACT:
        break;
    case BinaryOpType::MULTIPLY:
        break;
    case BinaryOpType::DIVIDE:
        break;
    default:
        return nullptr;
    }

    return nullptr;
}

std::optional<std::unique_ptr<IAstNode>> try_reduce_additive_op(
    AstBinaryArithmeticOp* self,
    AstExpression* left_lit,
    AstExpression* right_lit
)
{
    /*if (const auto lhstd::shared_ptr_i = dynamic_cast<AstIntLiteral*>(left_lit);
        lhstd::shared_ptr_i != nullptr
    )
    {
        // For integers, we do allow addition of other numeric types, though
        // this will change the resulting type. E.g., adding a float to an int
        // will result in a float (32/64 bit)

        auto lval = lval_ptr->value();

        if (right_lit->type() == LiteralType::FLOAT) {}
    }*/

    return std::nullopt;
}

IAstNode* AstBinaryArithmeticOp::reduce()
{
    /*if (!this->is_reducible()) return this;


    if (const auto lhstd::shared_ptr_i = dynamic_cast<AstIntLiteral*>(this->left.get());
        lhstd::shared_ptr_i != nullptr
    )
    {
        // For integers, we do allow addition of other numeric types, though
        // this will change the resulting type. E.g., adding a float to an int
        // will result in a float (32/64 bit)

        auto lval = lval_ptr->value();

        if (right_lit->type() == LiteralType::FLOAT) {}
    }

    return std::nullopt;*/

    return this;
}
