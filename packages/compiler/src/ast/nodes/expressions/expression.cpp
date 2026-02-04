#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

#include "ast/flags.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

llvm::Value* AstExpression::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* ir_builder
)
{
    throw parsing_error("Expression codegen not implemented, this must be implemented by subclasses");
}

std::string AstExpression::to_string()
{
    return "AnonymousExpression";
}

std::unique_ptr<AstExpression> stride::ast::parse_inline_expression_part(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    if (auto lit = parse_literal_optional(registry, set);
        lit.has_value())
    {
        return std::move(lit.value());
    }

    if (auto unary = parse_binary_unary_op(registry, set);
        unary.has_value())
    {
        return std::move(unary.value());
    }

    // If the next token is a '(', we'll try to descend into it
    // until we find another one, e.g. `(1 + (2 * 3))` with nested parentheses
    if (set.peek_next_eq(TokenType::LPAREN))
    {
        set.next();
        // TODO: Potentially fix possibility of stack overflow if expression is too large
        auto expr = parse_inline_expression_part(registry, set);
        set.expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    if (set.peek_next_eq(TokenType::IDENTIFIER))
    {
        /// Function invocations, e.g., `<identifier>(...)`
        if (set.peek(1).get_type() == TokenType::LPAREN)
        {
            return parse_function_call(registry, set);
        }

        if (auto reassignment = parse_variable_reassignment(registry, set);
            reassignment.has_value())
        {
            return std::move(reassignment.value());
        }

        /// Regular identifier parsing; can be variable reference
        const auto reference_token = set.next();
        std::string identifier_name = reference_token.get_lexeme();
        std::string internal_name = identifier_name;

        if (const auto variable_definition = registry->lookup_variable(identifier_name);
            variable_definition != nullptr)
        {
            internal_name = variable_definition->get_internal_symbol_name();
        }

        auto identifier = std::make_unique<AstIdentifier>(
            set.get_source(),
            reference_token.get_source_position(),
            registry,
            identifier_name,
            internal_name
        );

        if (set.peek_next_eq(TokenType::LSQUARE_BRACKET))
        {
            return parse_array_member_accessor(registry, set, std::move(identifier));
        }

        if (is_member_accessor(identifier.get(), set))
        {
            return parse_chained_member_access(registry, set, std::move(identifier));
        }

        return std::move(identifier);
    }

    set.throw_error("Invalid token found in expression");
}

std::optional<std::unique_ptr<AstExpression>> parse_logical_operation_optional(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs
)
{
    const auto reference_token = set.peek_next();

    if (auto logical_op = get_logical_op_type(reference_token.get_type()); logical_op.has_value())
    {
        set.next();

        auto rhs = parse_inline_expression_part(registry, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstLogicalOp>(
            set.get_source(),
            reference_token.get_source_position(),
            registry,
            std::move(lhs),
            logical_op.value(),
            std::move(rhs)
        );
    }

    return lhs;
}

// Will yield `lhs` if no comparative operation is found
std::optional<std::unique_ptr<AstExpression>> parse_comparative_operation_optional(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs
)
{
    const auto reference_token = set.peek_next();

    if (auto comparative_op = get_comparative_op_type(reference_token.get_type()); comparative_op.has_value())
    {
        set.next();

        auto rhs = parse_inline_expression_part(registry, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstComparisonOp>(
            set.get_source(),
            reference_token.get_source_position(),
            registry,
            std::move(lhs),
            comparative_op.value(),
            std::move(rhs)
        );
    }

    return lhs;
}

std::unique_ptr<AstExpression> stride::ast::parse_expression_extended(
    const int expression_type_flags,
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    if (!set.has_next()) return nullptr;

    // Will try to parse <name>::{ ... }
    if (is_struct_initializer(set))
    {
        return parse_struct_initializer(registry, set);
    }

    // Will try to parse [ ... ]
    if (is_array_initializer(set))
    {
        return parse_array_initializer(registry, set);
    }

    // let <name>: <type> = <...>
    if (is_variable_declaration(set))
    {
        return parse_variable_declaration(
            expression_type_flags,
            registry,
            set
        );
    }

    auto lhs = parse_inline_expression_part(registry, set);
    if (!lhs)
    {
        set.throw_error("Unexpected token in expression");
    }

    // Attempt to parse arithmetic binary operations first

    // If we have a result from arithmetic parsing, update lhs.
    // Note: parse_arithmetic_binary_operation_optional returns the lhs if no arithmetic op is found,
    // so it effectively passes through unless an error occurs (returns nullopt).
    if (auto arithmetic_result = parse_arithmetic_binary_operation_optional(registry, set, std::move(lhs), 1);
        arithmetic_result.has_value())
    {
        lhs = std::move(arithmetic_result.value());
    }
    else
    {
        // This case handles errors during arithmetic parsing (e.g. valid LHS but invalid RHS)
        set.throw_error("Invalid arithmetic expression");
    }

    // These methods optionally wrap the existing expression in another expression, e.g., Expr<lhs> -> ComparativeExpr<Expr<lhs>, Expr<rhs>>
    if (auto comparative_expr = parse_comparative_operation_optional(registry, set, std::move(lhs));
        comparative_expr.has_value())
    {
        return std::move(comparative_expr.value());
    }

    // Similar case here
    if (auto logical_expr = parse_logical_operation_optional(registry, set, std::move(lhs)))
    {
        return std::move(logical_expr.value());
    }

    set.throw_error("Unexpected token in expression");
}

/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    auto expr = parse_expression_extended(
        SRFLAG_EXPR_TYPE_STANDALONE,
        registry,
        set
    );

    set.expect(TokenType::SEMICOLON, "Expected ';' after expression");

    return expr;
}

std::unique_ptr<AstExpression> stride::ast::parse_inline_expression(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    return parse_expression_extended(
        SRFLAG_EXPR_TYPE_INLINE,
        registry,
        set
    );
}

/// TODO: Implement
std::string stride::ast::parse_property_accessor_statement(
    [[maybe_unused]] const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::IDENTIFIER);
    auto identifier_name = reference_token.get_lexeme();

    int iterations = 0;
    while (true)
    {
        if (set.peek_next_eq(TokenType::DOT) && set.peek_eq(TokenType::IDENTIFIER, 1))
        {
            set.next();
            const auto accessor_token = set.expect(TokenType::IDENTIFIER);
            identifier_name += SR_PROPERTY_ACCESSOR_SEPARATOR + accessor_token.get_lexeme();
        }
        else break;

        if (++iterations > SR_EXPRESSION_MAX_IDENTIFIER_RESOLUTION)
        {
            set.throw_error("Maximum identifier resolution exceeded");
        }
    }

    return identifier_name;
}

bool stride::ast::is_variable_reassignment_statement(const TokenSet& set)
{
    return set.peek_next_eq(TokenType::IDENTIFIER);
}
