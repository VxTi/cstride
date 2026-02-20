#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

#include <format>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::optional<BinaryOpType> stride::ast::get_binary_op_type(
    const TokenType type)
{
    switch (type)
    {
    case TokenType::STAR:
        return BinaryOpType::MULTIPLY;
    case TokenType::PLUS:
        return BinaryOpType::ADD;
    case TokenType::MINUS:
        return BinaryOpType::SUBTRACT;
    case TokenType::SLASH:
        return BinaryOpType::DIVIDE;
    case TokenType::PERCENT:
        return BinaryOpType::MODULO;
    default:
        return std::nullopt;
    }
}

int stride::ast::get_binary_operator_precedence(const BinaryOpType type)
{
    switch (type)
    {
    case BinaryOpType::POWER:
        return 3;
    case BinaryOpType::MULTIPLY:
    case BinaryOpType::DIVIDE:
    case BinaryOpType::MODULO:
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
    case BinaryOpType::ADD:
        return "+";
    case BinaryOpType::SUBTRACT:
        return "-";
    case BinaryOpType::MULTIPLY:
        return "*";
    case BinaryOpType::DIVIDE:
        return "/";
    case BinaryOpType::MODULO:
        return "%";
    default:
        return "";
    }
}

std::string AstBinaryArithmeticOp::to_string()
{
    return std::format(
        "BinaryOp({}, {}, {})",
        this->get_left()->to_string(),
        binary_op_to_str(this->get_op_type()),
        this->get_right()->to_string());
}

/**
 * This parses expressions that requires precedence, such as binary expressions.
 * These are binary expressions, e.g., 1 + 1, 1 - 1, 1 * 1, 1 / 1, 1 % 1
 */
std::optional<std::unique_ptr<AstExpression>>
stride::ast::parse_arithmetic_binary_operation_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs,
    const int min_precedence
)
{
    const size_t starting_offset = lhs->get_source_fragment().offset;
    int recursion_depth = 0;

    while (set.has_next())
    {
        const auto reference_token = set.peek_next();
        // First, we'll check if the next token is a binary operator
        if (auto binary_op = get_binary_op_type(reference_token.get_type());
            binary_op.has_value())
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
            // we'll return nullopt. This will indicate that the expression is incomplete or
            // invalid.
            auto rhs_opt_val = parse_binary_unary_op(context, set);
            if (!rhs_opt_val)
            {
                return std::nullopt;
            }
            auto rhs = std::move(rhs_opt_val.value());

            // If the followup token is also a binary expression,
            // we can try to parse it with higher precedence
            if (const auto next_op = get_binary_op_type(set.peek_next_type());
                next_op.has_value())
            {
                if (const int next_precedence = get_binary_operator_precedence(
                        next_op.value());
                    precedence < next_precedence)
                {
                    auto rhs_opt = parse_arithmetic_binary_operation_optional(
                        context,
                        set,
                        std::move(rhs),
                        precedence + 1
                    );
                    if (rhs_opt.has_value())
                    {
                        rhs = std::move(rhs_opt.value());
                    }
                    else
                    {
                        // We're unable to parse the next expression part, which is required for a
                        // binary operation.
                        return std::nullopt;
                    }
                }
            }

            const auto& rhs_pos = rhs->get_source_fragment();
            lhs = std::make_unique<AstBinaryArithmeticOp>(
                SourceFragment(
                    set.get_source(),
                    starting_offset,
                    rhs_pos.offset + rhs_pos.length - starting_offset
                ),
                context,
                std::move(lhs),
                binary_op.value(),
                std::move(rhs)
            );
        }
        else
        {
            return lhs;
        }

        if (++recursion_depth > MAX_RECURSION_DEPTH)
        {
            set.throw_error(
                "Maximum recursion depth exceeded when parsing binary arithmetic expression");
        }
    }
    return lhs;
}

llvm::Value* AstBinaryArithmeticOp::codegen(
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    llvm::Value* lhs = this->get_left()->codegen(module, builder);
    llvm::Value* rhs = this->get_right()->codegen(module, builder);

    if (!lhs || !rhs)
    {
        return nullptr;
    }

    // Determine if the result should be floating point
    const bool is_float =
        lhs->getType()->isFloatingPointTy() ||
        rhs->getType()->isFloatingPointTy();

    // Handle Integer Promotion (Int <-> Int)
    if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy())
    {
        const auto l_width = lhs->getType()->getIntegerBitWidth();

        if (const auto r_width = rhs->getType()->getIntegerBitWidth();
            l_width < r_width)
        {
            lhs = builder->CreateIntCast(
                lhs,
                rhs->getType(),
                true,
                "binop_sext"
            );
        }
        else if (r_width < l_width)
        {
            rhs = builder->CreateIntCast(
                rhs,
                lhs->getType(),
                true,
                "binop_sext"
            );
        }
    }
    // Handle Mixed Types (Int <-> Float) and Float Promotion
    else if (is_float)
    {
        if (lhs->getType()->isIntegerTy())
        {
            // Cast LHS integer to match RHS float type
            lhs = builder->CreateSIToFP(lhs, rhs->getType(), "sitofp_unary");
        }
        else if (rhs->getType()->isIntegerTy())
        {
            // Cast RHS integer to match LHS float type
            rhs = builder->CreateSIToFP(rhs, lhs->getType(), "sitofp_unary");
        }
        else if (lhs->getType() != rhs->getType())
        {
            // Promote smaller float to larger float (e.g. float -> double)
            if (lhs->getType()->getPrimitiveSizeInBits() < rhs->getType()->getPrimitiveSizeInBits())
            {
                lhs = builder->CreateFPExt(lhs, rhs->getType(), "fpext");
            }
            else
            {
                rhs = builder->CreateFPExt(rhs, lhs->getType(), "fpext");
            }
        }
    }

    switch (this->get_op_type())
    {
    case BinaryOpType::ADD:
        return is_float
            ? builder->CreateFAdd(lhs, rhs, "addtmp")
            : builder->CreateAdd(lhs, rhs, "addtmp");
    case BinaryOpType::SUBTRACT:
        return is_float
            ? builder->CreateFSub(lhs, rhs, "subtmp")
            : builder->CreateSub(lhs, rhs, "subtmp");
    case BinaryOpType::MULTIPLY:
        return is_float
            ? builder->CreateFMul(lhs, rhs, "multmp")
            : builder->CreateMul(lhs, rhs, "multmp");
    case BinaryOpType::DIVIDE:
        return is_float
            ? builder->CreateFDiv(lhs, rhs, "divtmp")
            : builder->CreateSDiv(lhs, rhs, "divtmp");
    case BinaryOpType::MODULO:
        return is_float
            ? builder->CreateFRem(lhs, rhs, "modtmp")
            : builder->CreateSRem(lhs, rhs, "modtmp");
    default:
        return nullptr;
    }
}

bool AstBinaryArithmeticOp::is_reducible()
{
    // A binary operation is reducible if either of both sides is reducible
    // A literal doesn't extend `IReducible`, though is by nature "reducible" in followup steps.
    return (is_literal_ast_node(this->get_left()) && is_literal_ast_node(this->get_right()))
        || this->get_left()->is_reducible()
        || this->get_right()->is_reducible();
}

IAstNode* AstBinaryArithmeticOp::reduce()
{
    return this;
}
