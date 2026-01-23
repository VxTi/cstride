#include <memory>

#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

std::unique_ptr<IAstInternalFieldType> resolve_expression_literal_internal_type(AstLiteral* literal)
{
    if (const auto* str = dynamic_cast<AstStringLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            str->source, str->source_offset, PrimitiveType::STRING, 1
        );
    }

    if (const auto* fp_lit = dynamic_cast<AstFpLiteral*>(literal))
    {
        auto type = fp_lit->bit_count() > 4 ? PrimitiveType::FLOAT64 : PrimitiveType::FLOAT32;
        return std::make_unique<AstPrimitiveFieldType>(
            fp_lit->source, fp_lit->source_offset, type, fp_lit->bit_count()
        );
    }

    if (const auto* int_lit = dynamic_cast<AstIntLiteral*>(literal))
    {
        auto type = int_lit->bit_count() > 32 ? PrimitiveType::INT64 : PrimitiveType::INT32;
        return std::make_unique<AstPrimitiveFieldType>(
            int_lit->source, int_lit->source_offset, type, int_lit->bit_count()
        );
    }

    if (const auto* char_lit = dynamic_cast<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            char_lit->source, char_lit->source_offset, PrimitiveType::CHAR, char_lit->bit_count()
        );
    }

    if (const auto* bool_lit = dynamic_cast<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            bool_lit->source, bool_lit->source_offset, PrimitiveType::BOOL, bool_lit->bit_count()
        );
    }

    throw std::runtime_error(
        stride::make_ast_error(*literal->source, literal->source_offset, "Unable to resolve expression literal type")
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::resolve_expression_internal_type(
    const std::shared_ptr<Scope>& scope,
    AstExpression* expr
)
{
    if (auto* literal = dynamic_cast<AstLiteral*>(expr))
    {
        return resolve_expression_literal_internal_type(literal);
    }

    if (const auto* identifier = dynamic_cast<AstIdentifier*>(expr))
    {
        const auto variable_def = scope->get_variable_def(identifier->get_name());
        if (variable_def == nullptr)
        {
            throw parsing_error(
                make_ast_error(*identifier->source, identifier->source_offset, "Variable not found in scope")
            );
        }

        // Assumes `get_type()` returns a pointer to a long-lived type.
        // If not, this needs a clone/copy API instead.
        return variable_def->get_type()->clone();
    }

    if (const auto* operation = dynamic_cast<AstBinaryArithmeticOp*>(expr))
    {
        auto lhs = resolve_expression_internal_type(scope, &operation->get_left());
        const auto rhs = resolve_expression_internal_type(scope, &operation->get_right());

        if (*lhs == *rhs)
        {
            return lhs;
        }

        return get_dominant_type(&*lhs, &*rhs);
    }

    if (const auto* operation = dynamic_cast<AstUnaryOp*>(expr))
    {
        return resolve_expression_internal_type(scope, &operation->get_operand());
    }

    if (dynamic_cast<AstLogicalOp*>(expr) || dynamic_cast<AstComparisonOp*>(expr))
    {
        // TODO: Do some validation on lhs and rhs; cannot compare strings with one another (yet)
        return std::make_unique<AstPrimitiveFieldType>(
            expr->source, expr->source_offset, PrimitiveType::BOOL, 1, 0
        );
    }

    if (const auto* operation = dynamic_cast<AstVariableReassignment*>(expr))
    {
        return resolve_expression_internal_type(scope, operation->get_value());
    }

    if (const auto* operation = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        const auto declared = operation->get_variable_type();
        const auto value = resolve_expression_internal_type(scope, operation->get_initial_value().get());

        // Both the expression type and the declared type are the same (e.g., let var: i32 = 10)
        // so we can just return the declared type.
        if (*declared == *value)
        {
            return std::unique_ptr<IAstInternalFieldType>(declared);
        }

        return get_dominant_type(declared, value.get());
    }

    throw parsing_error("Unable to resolve expression type" + expr->to_string());
}
