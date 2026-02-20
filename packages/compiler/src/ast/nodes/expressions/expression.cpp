#include "ast/nodes/expression.h"

#include "ast/nodes/blocks.h"
#include "ast/nodes/literal_values.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

using namespace stride::ast;

llvm::Value* AstExpression::codegen(
    const ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilder<>* ir_builder)
{
    throw parsing_error(
        "Expression codegen not implemented, this must be implemented by subclasses");
}

std::string AstExpression::to_string()
{
    return "AnonymousExpression";
}

std::unique_ptr<AstExpression> stride::ast::parse_inline_expression_part(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (auto lit = parse_literal_optional(context, set); lit.has_value())
    {
        return std::move(lit.value());
    }

    // Will try to parse <name>::{ ... }
    if (is_struct_initializer(set))
    {
        return parse_struct_initializer(context, set);
    }

    // Will try to parse [ ... ]
    if (is_array_initializer(set))
    {
        return parse_array_initializer(context, set);
    }

    // Could either be a function call, or object access
    if (set.peek_next_eq(TokenType::IDENTIFIER))
    {
        /// Regular identifier parsing; can be variable reference
        const auto reference_token = set.peek_next();
        // Mangled name including module, e.g., `Math__PI`
        const SymbolNameSegments name_segments =
            parse_segmented_identifier(set);
        const auto internal_name = resolve_internal_name(name_segments);

        auto identifier = std::make_unique<AstIdentifier>(
            context,
            Symbol(reference_token.get_source_fragment(), internal_name));

        if (auto reassignment = parse_variable_reassignment(context, internal_name, set);
            reassignment.has_value())
        {
            return std::move(reassignment.value());
        }

        /// Function invocations, e.g., `<identifier>(...)`, or `<module>::<identifier>`
        if (set.peek_next_eq(TokenType::LPAREN))
        {
            return parse_function_call(context, name_segments, set);
        }

        if (set.peek_next_eq(TokenType::LSQUARE_BRACKET))
        {
            return parse_array_member_accessor(
                context,
                set,
                std::move(identifier)
            );
        }

        if (is_member_accessor(identifier.get(), set))
        {
            return parse_chained_member_access(
                context,
                set,
                std::move(identifier)
            );
        }

        return std::move(identifier);
    }

    // If the next token is a '(', we'll try to descend into it
    // until we find another one, e.g. `(1 + (2 * 3))` with nested parentheses
    if (set.peek_next_eq(TokenType::LPAREN))
    {
        if (set.peek_eq(TokenType::IDENTIFIER, 1)
            && set.peek_eq(TokenType::COLON, 2))
        {
            return parse_lambda_fn_expression(context, set);
        }

        set.next();
        // Fixed: Use parse_inline_expression (full expression parser) instead of
        // parse_inline_expression_part to allow binary operations inside parentheses.
        auto expr = parse_inline_expression(context, set);
        set.expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    if (set.peek_next_eq(TokenType::THREE_DOTS))
    {
        const auto& ref = set.next();
        return std::make_unique<AstVariadicArgReference>(
            ref.get_source_fragment(),
            context
        );
    }

    set.throw_error("Invalid token found in expression");
}

/*
 * Helper functions for tiered expression parsing to ensure correct precedence.
 * Hierarchy: Logical > Comparison > Arithmetic > Unary > Atom
 */

std::unique_ptr<AstExpression> parse_arithmetic_tier(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    // 1. Term (Unary / Primary)
    auto lhs_opt = parse_binary_unary_op(context, set);
    if (!lhs_opt)
    {
        set.throw_error("Expected expression");
    }
    auto lhs = std::move(lhs_opt.value());

    // 2. Arithmetic Loop (handled by parse_arithmetic_binary_operation_optional)
    if (auto arith = parse_arithmetic_binary_operation_optional(
        context,
        set,
        std::move(lhs),
        1)
    )
    {
        return std::move(arith.value());
    }
    return lhs;
}

std::unique_ptr<AstExpression> parse_comparison_tier(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    auto lhs = parse_arithmetic_tier(context, set);

    while (auto op = get_comparative_op_type(set.peek_next_type()))
    {
        const auto token = set.next();
        auto rhs = parse_arithmetic_tier(context, set);
        lhs = std::make_unique<AstComparisonOp>(
            token.get_source_fragment(),
            context,
            std::move(lhs),
            op.value(),
            std::move(rhs)
        );
    }
    return lhs;
}

std::unique_ptr<AstExpression> parse_logical_tier(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    auto lhs = parse_comparison_tier(context, set);

    while (auto op = get_logical_op_type(set.peek_next_type()))
    {
        const auto token = set.next();
        auto rhs = parse_comparison_tier(context, set);
        lhs = std::make_unique<AstLogicalOp>(
            token.get_source_fragment(),
            context,
            std::move(lhs),
            op.value(),
            std::move(rhs)
        );
    }
    return lhs;
}

// Kept for backward compatibility / external usage if any, but now updated to use correct tiers for
// RHS
std::optional<std::unique_ptr<AstExpression>> parse_logical_operation_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs)
{
    const auto reference_token = set.peek_next();

    if (auto logical_op = get_logical_op_type(reference_token.get_type());
        logical_op.has_value())
    {
        set.next();

        auto rhs = parse_comparison_tier(context, set);
        // Note: calling parse_comparison_tier here is safer than parse_inline_expression_part

        return std::make_unique<AstLogicalOp>(
            reference_token.get_source_fragment(),
            context,
            std::move(lhs),
            logical_op.value(),
            std::move(rhs));
    }

    return lhs;
}

// Kept for backward compatibility / external usage if any
std::optional<std::unique_ptr<AstExpression>> parse_comparative_operation_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs)
{
    const auto reference_token = set.peek_next();

    if (auto comparative_op = get_comparative_op_type(
            reference_token.get_type());
        comparative_op.has_value())
    {
        set.next();

        auto rhs = parse_arithmetic_tier(context, set);

        return std::make_unique<AstComparisonOp>(
            reference_token.get_source_fragment(),
            context,
            std::move(lhs),
            comparative_op.value(),
            std::move(rhs)
        );
    }

    return lhs;
}

std::unique_ptr<AstExpression> parse_expression_internal(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (!set.has_next())
    {
        set.throw_error("Unexpected end of input while parsing expression");
    }

    return parse_logical_tier(context, set);
}


/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    auto expr = parse_expression_internal(context, set);

    set.expect(TokenType::SEMICOLON, "Expected ';' after expression");

    return expr;
}

std::unique_ptr<AstExpression> stride::ast::parse_inline_expression(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    return parse_expression_internal(context, set);
}

SymbolNameSegments stride::ast::parse_segmented_identifier(TokenSet& set)
{
    std::vector<std::string> segments = {};

    segments.push_back(set.expect(TokenType::IDENTIFIER).get_lexeme());

    while (set.peek_next_eq(TokenType::DOUBLE_COLON))
    {
        set.next();
        const auto subseq_iden = set.expect(
            TokenType::IDENTIFIER,
            "Expected identifier in module accessor"
        );
        segments.push_back(subseq_iden.get_lexeme());
    }

    return segments;
}
