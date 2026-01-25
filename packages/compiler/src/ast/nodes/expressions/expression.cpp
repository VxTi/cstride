#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

#include "ast/flags.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

llvm::Value* AstExpression::codegen(
    const std::shared_ptr<Scope>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* irBuilder
)
{
    throw parsing_error("Expression codegen not implemented, this must be implemented by subclasses");
}

std::string AstExpression::to_string()
{
    return "AnonymousExpression";
}

std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression_part(
    const std::shared_ptr<Scope>& scope,
    TokenSet& set
)
{
    if (auto lit = parse_literal_optional(scope, set);
        lit.has_value())
    {
        return std::move(lit.value());
    }

    if (auto unary = parse_binary_unary_op(scope, set);
        unary.has_value())
    {
        return std::move(unary.value());
    }

    if (auto reassignment = parse_variable_reassignment(scope, set);
        reassignment.has_value())
    {
        return std::move(reassignment.value());
    }

    if (set.peak_next_eq(TokenType::LPAREN))
    {
        set.next();
        auto expr = parse_standalone_expression_part(scope, set);
        set.expect(TokenType::RPAREN);
        return expr;
    }

    if (set.peak_next_eq(TokenType::IDENTIFIER))
    {
        /// Function invocations
        if (set.peak(1).type == TokenType::LPAREN)
        {
            return parse_function_call(scope, set);
        }

        /// Regular identifier parsing; can be variable reference
        const auto reference_token = set.next();
        std::string identifier_name = reference_token.lexeme;
        std::string internal_name;

        if (const auto variable_definition = scope->field_lookup(identifier_name);
            variable_definition != nullptr)
        {
            internal_name = variable_definition->get_internal_symbol_name();
        }

        return std::make_unique<AstIdentifier>(
            set.source(),
            reference_token.offset,
            scope,
            identifier_name,
            internal_name
        );
    }

    set.throw_error("Invalid expression");
}

/**
 * Parsing of an expression that does not require precedence, but has a LHS and RHS, and an operator.
 * This can be either binary expressions, e.g., 1 + 1, or comparative expressions, e.g., 1 < 2
 */
std::optional<std::unique_ptr<AstExpression>> parse_logical_or_comparative_op(
    const std::shared_ptr<Scope>& scope,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs
)
{
    const auto reference_token = set.peak_next();

    if (auto logical_op = get_logical_op_type(reference_token.type); logical_op.has_value())
    {
        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstLogicalOp>(
            set.source(),
            reference_token.offset,
            scope,
            std::move(lhs),
            logical_op.value(),
            std::move(rhs)
        );
    }

    if (auto comparative_op = get_comparative_op_type(reference_token.type); comparative_op.has_value())
    {
        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstComparisonOp>(
            set.source(),
            reference_token.offset,
            scope,
            std::move(lhs),
            comparative_op.value(),
            std::move(rhs)
        );
    }

    return lhs;
}


std::unique_ptr<AstExpression> stride::ast::parse_expression_extended(
    const int expression_type_flags,
    const std::shared_ptr<Scope>& scope,
    TokenSet& set
)
{
    if (is_variable_declaration(set))
    {
        return parse_variable_declaration(
            expression_type_flags,
            scope,
            set
        );
    }

    auto lhs = parse_standalone_expression_part(scope, set);
    if (!lhs)
    {
        set.throw_error("Unexpected token in expression");
    }

    // Attempt to parse arithmetic binary operations first

    // If we have a result from arithmetic parsing, update lhs.
    // Note: parse_arithmetic_binary_op returns the lhs if no arithmetic op is found,
    // so it effectively passes through unless an error occurs (returns nullopt).
    if (auto arithmetic_result = parse_arithmetic_binary_op(scope, set, std::move(lhs), 1); arithmetic_result.
        has_value())
    {
        lhs = std::move(arithmetic_result.value());
    }
    else
    {
        // This case handles errors during arithmetic parsing (e.g. valid LHS but invalid RHS)
        set.throw_error("Invalid arithmetic expression");
    }

    // Now attempt to parse logical or comparative operations using the result

    if (auto logical_result = parse_logical_or_comparative_op(scope, set, std::move(lhs));
        logical_result.has_value())
    {
        return std::move(logical_result.value());
    }

    set.throw_error("Unexpected token in expression");
}

/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression(
    const std::shared_ptr<Scope>& scope,
    TokenSet& set
)
{
    return parse_expression_extended(
        SRFLAG_EXPR_TYPE_STANDALONE,
        scope,
        set
    );
}

std::string stride::ast::parse_property_accessor_statement(
    const std::shared_ptr<Scope>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::IDENTIFIER);
    auto identifier_name = reference_token.lexeme;

    int iterations = 0;
    while (true)
    {
        if (set.peak_next_eq(TokenType::DOT) && set.peak_eq(TokenType::IDENTIFIER, 1))
        {
            set.next();
            const auto accessor_token = set.expect(TokenType::IDENTIFIER);
            identifier_name += SR_PROPERTY_ACCESSOR_SEPARATOR + accessor_token.lexeme;
        }
        else break;

        if (++iterations > SR_EXPRESSION_MAX_IDENTIFIER_RESOLUTION)
        {
            set.throw_error("Maximum identifier resolution exceeded");
        }
    }

    return identifier_name;
}

bool stride::ast::is_property_accessor_statement(const TokenSet& set)
{
    const bool initial_identifier = set.peak_next_eq(TokenType::IDENTIFIER);
    const bool is_followup_accessor = set.peak_eq(TokenType::DOT, 1)
        && set.peak_eq(TokenType::IDENTIFIER, 2);

    return initial_identifier && (is_followup_accessor || true);
}
