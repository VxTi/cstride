#include "ast/nodes/expression.h"

#include "errors.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

llvm::Value* IAstExpression::codegen(
    SymbolTable* symbol_table,
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    throw stride_error(
        "Expression codegen not implemented, this must be implemented by subclasses");
}

std::string IAstExpression::to_string()
{
    return "AnonymousExpression";
}

std::unique_ptr<IAstExpression> stride::ast::parse_inline_expression_part(
    TokenSet& set
)
{
    std::unique_ptr<IAstExpression> result;

    if (auto lit = parse_literal_optional(set);
        lit.has_value())
    {
        result = std::move(lit.value());
    }
    // Will try to parse <name>::{ ... }
    else if (is_struct_initializer(set))
    {
        result = parse_object_initializer(set);
    }
    // Will try to parse [ ... ]
    else if (is_array_initializer(set))
    {
        result = parse_array_initializer(set);
    }
    // Could either be a function call, or object/array access
    else if (set.peek_next_eq(TokenType::IDENTIFIER))
    {
        const auto reference_token = set.peek_next();
        // Mangled name including module, e.g., `Math__PI`
        auto identifier = parse_segmented_identifier(
            set,
            "Expected identifier in expression"
        );

        if (auto reassignment = parse_variable_reassignment(identifier.get(), set);
            reassignment.has_value())
        {
            return std::move(reassignment.value());
        }

        // Named function invocations, e.g., `<identifier>(...)` or `<module>::<identifier>(...)`
        if (is_direct_function_call(set))
        {
            result = parse_function_call(identifier.get(), set);
        }
        else
        {
            result = std::move(identifier);
        }
    }
    // If the next token is a '(', we'll try to descend into it
    // until we find another one, e.g. `(1 + (2 * 3))` with nested parentheses
    else if (set.peek_next_eq(TokenType::LPAREN))
    {
        if ((set.peek_eq(TokenType::IDENTIFIER, 1) // Checks for "(<identifier>: ..."
                && set.peek_eq(TokenType::COLON, 2))
            || (set.peek_eq(TokenType::RPAREN, 1) && // Checks for "():"
                set.peek_eq(TokenType::COLON, 2)))
        {
            result = parse_anonymous_fn_expression(set);
        }
        else
        {
            set.next();
            // Fixed: Use parse_inline_expression (full expression parser) instead of
            // parse_inline_expression_part to allow binary operations inside parentheses.
            auto expr = parse_inline_expression(set);
            // TODO: If we have a comma next, it might be a tuple expression
            set.expect(TokenType::RPAREN, "Expected ')' after expression");
            result = std::move(expr);
        }
    }
    else if (set.peek_next_eq(TokenType::THREE_DOTS))
    {
        const auto& ref = set.next();
        result = std::make_unique<AstVariadicArgReference>(ref.get_source_position());
    }
    else
    {
        set.throw_error("Invalid token found in expression");
    }

    // Unified postfix operator loop:
    // Handles `.member`, `[index]`, and `(args)` chaining on any primary expression.
    // Builds a left-recursive tree so each step's base is the result of the previous step.
    int recursion_depth = 0;
    while (true)
    {
        if (is_member_accessor(set))
        {
            result = parse_chained_member_access(set, std::move(result));
        }
        else if (set.peek_next_eq(TokenType::LSQUARE_BRACKET))
        {
            result = parse_array_member_accessor(set, std::move(result));
        }
        else if (set.peek_next_eq(TokenType::LPAREN))
        {
            result = parse_indirect_call(set, std::move(result));
        }
        else
        {
            break;
        }
        if (++recursion_depth > MAX_RECURSION_DEPTH)
        {
            set.throw_error("Expression too complex");
        }
    }

    return result;
}

/*
 * Helper functions for tiered expression parsing to ensure correct precedence.
 * Hierarchy: Logical > Comparison > Arithmetic > Unary > Atom
 */

std::unique_ptr<IAstExpression> parse_arithmetic_tier(TokenSet& set)
{
    // 1. Term (Unary / Primary)
    auto lhs_opt = parse_binary_unary_op(set);
    if (!lhs_opt)
    {
        set.throw_error("Expected expression");
    }
    auto lhs = std::move(lhs_opt.value());

    // Highest precedence: Type Cast (as)
    while (auto cast_expr = parse_type_cast_op(set, lhs.get()))
    {
        lhs = std::move(cast_expr.value());
    }

    // 2. Arithmetic Loop (handled by parse_arithmetic_binary_operation_optional)
    if (auto arith = parse_arithmetic_binary_operation_optional(set, std::move(lhs), 1)
    )
    {
        return std::move(arith.value());
    }
    return lhs;
}

std::unique_ptr<IAstExpression> parse_comparison_tier(
    TokenSet& set
)
{
    auto lhs = parse_arithmetic_tier(set);

    while (auto op = get_comparative_op_type(set.peek_next_type()))
    {
        const auto token = set.next();
        auto rhs = parse_arithmetic_tier(set);
        lhs = std::make_unique<AstComparisonOp>(
            stride::SourcePosition::join(lhs->get_source_position(), rhs->get_source_position()),
            std::move(lhs),
            op.value(),
            std::move(rhs)
        );
    }
    return lhs;
}

std::unique_ptr<IAstExpression> parse_logical_tier(TokenSet& set)
{
    auto lhs = parse_comparison_tier(set);

    while (auto op = get_logical_op_type(set.peek_next_type()))
    {
        const auto token = set.next();
        auto rhs = parse_comparison_tier(set);
        lhs = std::make_unique<AstLogicalOp>(
            stride::SourcePosition::join(lhs->get_source_position(), rhs->get_source_position()),
            std::move(lhs),
            op.value(),
            std::move(rhs)
        );
    }
    return lhs;
}

// Kept for backward compatibility / external usage if any, but now updated to use correct tiers for
// RHS
std::optional<std::unique_ptr<IAstExpression>> parse_logical_operation_optional(
    TokenSet& set,
    std::unique_ptr<IAstExpression> lhs)
{
    const auto reference_token = set.peek_next();

    if (auto logical_op = get_logical_op_type(reference_token.get_type());
        logical_op.has_value())
    {
        set.next();

        auto rhs = parse_comparison_tier(set);
        // Note: calling parse_comparison_tier here is safer than parse_inline_expression_part

        return std::make_unique<AstLogicalOp>(
            reference_token.get_source_position(),
            std::move(lhs),
            logical_op.value(),
            std::move(rhs));
    }

    return lhs;
}

// Kept for backward compatibility / external usage if any
std::optional<std::unique_ptr<IAstExpression>> parse_comparative_operation_optional(
    TokenSet& set,
    std::unique_ptr<IAstExpression> lhs)
{
    const auto reference_token = set.peek_next();

    if (auto comparative_op = get_comparative_op_type(
            reference_token.get_type());
        comparative_op.has_value())
    {
        set.next();

        auto rhs = parse_arithmetic_tier(set);

        return std::make_unique<AstComparisonOp>(
            reference_token.get_source_position(),
            std::move(lhs),
            comparative_op.value(),
            std::move(rhs)
        );
    }

    return lhs;
}

std::unique_ptr<IAstExpression> parse_expression_internal(TokenSet& set)
{
    if (!set.has_next())
    {
        set.throw_error("Unexpected end of input while parsing expression");
    }

    return parse_logical_tier(set);
}

/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<IAstExpression> stride::ast::parse_standalone_expression(
    TokenSet& set
)
{
    auto expr = parse_expression_internal(set);

    set.expect(TokenType::SEMICOLON, "Expected ';' after expression");

    return expr;
}

std::unique_ptr<IAstExpression> stride::ast::parse_inline_expression(
    TokenSet& set
)
{
    return parse_expression_internal(set);
}

std::unique_ptr<AstIdentifier> stride::ast::parse_segmented_identifier(
    TokenSet& set,
    const std::string& error_message)
{
    std::vector<std::string> segments;

    const auto initial_identifier = set.expect(TokenType::IDENTIFIER, error_message);
    segments.push_back(initial_identifier.get_lexeme());

    std::optional<SourcePosition> last_fragment = std::nullopt;

    while (set.peek_eq(TokenType::DOUBLE_COLON, 0)
        && set.peek_eq(TokenType::IDENTIFIER, 1))
    {
        set.next();
        const auto subseq_iden = set.expect(
            TokenType::IDENTIFIER,
            error_message
        );
        segments.push_back(subseq_iden.get_lexeme());
        last_fragment = subseq_iden.get_source_position();
    }

    const auto source_pos = last_fragment.has_value()
        ? SourcePosition::join(initial_identifier.get_source_position(), last_fragment.value())
        : initial_identifier.get_source_position();

    return std::make_unique<AstIdentifier>(
        Symbol(source_pos, resolve_internal_name(segments))
    );
}
