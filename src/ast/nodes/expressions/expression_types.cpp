#include <memory>

#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

/**
 * Resolves the internal field type for a given literal expression.
 *
 * This function examines a literal AST node and determines its corresponding internal field type
 * representation. It performs runtime type identification (via dynamic_cast) to determine the
 * specific kind of literal (string, floating-point, integer, character, or boolean) and creates
 * an appropriate AstPrimitiveFieldType instance with the correct primitive type and bit count.
 *
 * The function handles the following literal types:
 * - AstStringLiteral: Resolves to STRING primitive type with bit count of 1
 * - AstFpLiteral: Resolves to FLOAT32 or FLOAT64 depending on bit count (>4 bytes = FLOAT64)
 * - AstIntLiteral: Resolves to INT32 or INT64 depending on bit count (>32 bits = INT64)
 * - AstCharLiteral: Resolves to CHAR primitive type with the literal's bit count
 * - AstBooleanLiteral: Resolves to BOOL primitive type with the literal's bit count
 *
 * @param scope The current scope context (used for symbol resolution, though not directly used in this function)
 * @param literal Pointer to the AstLiteral node whose type needs to be resolved
 * @return A unique pointer to an IAstInternalFieldType representing the resolved type
 * @throws std::runtime_error If the literal type cannot be resolved (unknown literal subtype)
 */
std::unique_ptr<IAstInternalFieldType> resolve_expression_literal_internal_type(
    std::shared_ptr<Scope> scope, AstLiteral* literal)
{
    // "string" -> <string> (primitive)
    if (const auto* str = dynamic_cast<AstStringLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            str->source,
            str->source_offset,
            PrimitiveType::STRING,
            1
        );
    }

    // 1.325 -> <fp32 / fp64>
    if (const auto* fp_lit = dynamic_cast<AstFpLiteral*>(literal))
    {
        auto type = PrimitiveType::FLOAT32;
        if (fp_lit->bit_count() > 4)
        {
            type = PrimitiveType::FLOAT64;
        }

        return std::make_unique<AstPrimitiveFieldType>(
            fp_lit->source,
            fp_lit->source_offset,
            type,
            fp_lit->bit_count()
        );
    }

    // 1 -> <int32 / int64>
    if (const auto* int_lit = dynamic_cast<AstIntLiteral*>(literal))
    {
        auto type = int_lit->bit_count() > 32 ? PrimitiveType::INT64 : PrimitiveType::INT32;
        return std::make_unique<AstPrimitiveFieldType>(
            int_lit->source,
            int_lit->source_offset,
            type,
            int_lit->bit_count()
        );
    }

    // 'a' -> <char>
    if (const auto* char_lit = dynamic_cast<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            char_lit->source,
            char_lit->source_offset,
            PrimitiveType::CHAR,
            char_lit->bit_count()
        );
    }

    // true / false -> <bool>
    if (const auto* bool_lit = dynamic_cast<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            bool_lit->source,
            bool_lit->source_offset,
            PrimitiveType::BOOL,
            bool_lit->bit_count()
        );
    }

    throw std::runtime_error(
        stride::make_ast_error(
            *literal->source,
            literal->source_offset,
            "Unable to resolve expression literal type"
        )
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::resolve_expression_internal_type(
    const std::shared_ptr<Scope>& scope,
    AstExpression* expr
)
{
    // If the provided expression is already an IAstInternalFieldType, we can easily resolve it
    if (auto* literal = dynamic_cast<AstLiteral*>(expr))
    {
        return resolve_expression_literal_internal_type(scope, literal);
    }

    if (auto* identifier = dynamic_cast<AstIdentifier*>(expr))
    {
        // Look up type in scope
        const auto variable_def = scope->get_variable_def(identifier->get_name());
        if (variable_def == nullptr)
        {
            throw parsing_error(
                make_ast_error(
                    *identifier->source,
                    identifier->source_offset,
                    "Variable not found in scope"
                )
            );
        }

        return std::unique_ptr<IAstInternalFieldType>(variable_def->get_type());
    }

    if (const auto* operation = dynamic_cast<AstBinaryArithmeticOp*>(expr))
    {
        std::unique_ptr<IAstInternalFieldType> lhs_type = resolve_expression_internal_type(
            scope, &operation->get_left());
        const std::unique_ptr<IAstInternalFieldType> rhs_type = resolve_expression_internal_type(
            scope, &operation->get_right());

        // If both types are equal, then we know we've got the final type.
        if (*lhs_type == *rhs_type)
        {
            return std::move(lhs_type);
        }

        // Otherwise, we'll have to check which type is dominant.
        // For example, if we receive an arithmetic operation with two integer types of different
        // bit sizes, we'll return the integer type with the larger size.
        // This ensures we reserve the right amount of memory.
        return get_dominant_type(lhs_type.get(), rhs_type.get());
    }

    // We continue the same process as before for other expression types.


    // For unary expressions it's quite straight-forward; we'll just resolve the operand type.
    if (const auto* operation = dynamic_cast<AstUnaryOp*>(expr))
    {
        return resolve_expression_internal_type(scope, &operation->get_operand());
    }

    if (dynamic_cast<AstLogicalOp*>(expr) || dynamic_cast<AstComparisonOp*>(expr))
    {
        // For logical operations (&&, ||) and comparative ops, the return type is always a boolean.
        return std::make_unique<AstPrimitiveFieldType>(
            expr->source,
            expr->source_offset,
            PrimitiveType::BOOL,
            1,
            0
        );
    }

    if (const auto* operation = dynamic_cast<AstVariableReassignment*>(expr))
    {
        return resolve_expression_internal_type(scope, operation->get_value());
    }

    if (const auto* operation = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        const auto declared_type = operation->get_variable_type();
        const auto value_type = resolve_expression_internal_type(scope, operation->get_initial_value().get());

        if (*declared_type == *value_type)
        {
            return declared_type->clone();
        }

        return get_dominant_type(declared_type, value_type.get());
    }

    throw parsing_error("Unable to resolve expression type" + expr->to_string());
}