#include <memory>

#include "ast/nodes/functions.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

std::unique_ptr<IAstInternalFieldType> infer_expression_literal_type(AstLiteral* literal)
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

std::unique_ptr<IAstInternalFieldType> infer_function_call_return_type(
    const std::shared_ptr<Scope>& scope,
    const AstFunctionCall* fn_call)
{
    const auto regular_name = fn_call->get_function_name();
    // If we can simply get the function definition from the scope, we can return its type.
    if (const auto fn_def = scope->get_function_def(regular_name); fn_def != nullptr)
    {
        return fn_def->get_return_type()->clone();
    }

    // Otherwise, it's likely that we'll have to figure out its internal name ourselves based on the inferred
    // types from the callee's arguments
    std::vector<std::unique_ptr<IAstInternalFieldType>> resolved_arg_types;
    std::vector<IAstInternalFieldType*> parameter_types;

    const auto& args = fn_call->get_arguments();
    resolved_arg_types.reserve(args.size());
    parameter_types.reserve(args.size());

    for (const auto& arg : args)
    {
        auto expr_type = infer_expression_type(scope, arg.get());
        parameter_types.push_back(expr_type.get());
        resolved_arg_types.push_back(std::move(expr_type));
    }

    const auto internal_name = resolve_internal_function_name(parameter_types, regular_name);
    const auto definition = scope->get_function_def(internal_name);

    if (definition != nullptr)
    {
        return definition->get_return_type()->clone();
    }

    throw stride::parsing_error(
        stride::make_ast_error(
            *fn_call->source,
            fn_call->source_offset,
            std::format(
                "Unable to resolve function invocation return type for function '{}'",
                fn_call->get_function_name()
            )
        )
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_expression_type(
    const std::shared_ptr<Scope>& scope,
    AstExpression* expr
)
{
    if (auto* literal = dynamic_cast<AstLiteral*>(expr))
    {
        return infer_expression_literal_type(literal);
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
        auto lhs = infer_expression_type(scope, &operation->get_left());
        const auto rhs = infer_expression_type(scope, &operation->get_right());

        if (*lhs == *rhs)
        {
            return lhs;
        }

        return get_dominant_type(&*lhs, &*rhs);
    }

    if (const auto* operation = dynamic_cast<AstUnaryOp*>(expr))
    {
        return infer_expression_type(scope, &operation->get_operand());
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
        return infer_expression_type(scope, operation->get_value());
    }

    if (const auto* operation = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        const auto declared = operation->get_variable_type();
        const auto value = infer_expression_type(scope, operation->get_initial_value().get());

        // Both the expression type and the declared type are the same (e.g., let var: i32 = 10)
        // so we can just return the declared type.
        if (*declared == *value)
        {
            return std::unique_ptr<IAstInternalFieldType>(declared);
        }

        return get_dominant_type(declared, value.get());
    }

    if (const auto* fn_call = dynamic_cast<AstFunctionCall*>(expr))
    {
        return infer_function_call_return_type(scope, fn_call);
    }

    throw parsing_error("Unable to resolve expression type" + expr->to_string());
}
