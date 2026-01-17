#include "ast/nodes/expression.h"

using namespace stride::ast;

bool requires_identifier_operand(const UnaryOpType op)
{
    switch (op)
    {
    case UnaryOpType::LOGICAL_NOT: // !<123>
    case UnaryOpType::NEGATE:      // ~<123>
        return false;
    // For other unary types, we require identifiers, as this references some other value in memory
    default: return true;
    }
}

bool is_lhs_unary_op(const UnaryOpType op)
{
    switch (op)
    {
    case UnaryOpType::LOGICAL_NOT:
    case UnaryOpType::NEGATE:
    case UnaryOpType::COMPLEMENT:
    case UnaryOpType::INCREMENT:
    case UnaryOpType::DECREMENT:
    case UnaryOpType::DEREFERENCE:
    case UnaryOpType::ADDRESS_OF:
        return true;
    default: return false;
    }
}

std::optional<UnaryOpType> stride::ast::get_unary_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::BANG: return UnaryOpType::LOGICAL_NOT;
    case TokenType::MINUS: return UnaryOpType::NEGATE;
    case TokenType::TILDE: return UnaryOpType::COMPLEMENT;
    case TokenType::DOUBLE_PLUS: return UnaryOpType::INCREMENT;
    case TokenType::DOUBLE_MINUS: return UnaryOpType::DECREMENT;
    case TokenType::STAR: return UnaryOpType::DEREFERENCE;
    case TokenType::AMPERSAND: return UnaryOpType::ADDRESS_OF;
    default: break;
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<AstExpression>> stride::ast::parse_binary_unary_op(const Scope& scope, TokenSet& set)
{
    const auto next = set.peak_next();
    if (const auto op_type = get_unary_op_type(next.type); op_type.has_value())
    {
        set.next();
        return std::make_unique<AstUnaryOp>(op_type.value(), parse_standalone_expression_part(scope, set));
    }
}
