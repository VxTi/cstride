#include "ast/casting.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/return_statement.h"

#include <format>

using namespace stride::ast;

void IAstFunction::validate(SymbolTable* symbol_table)
{
    // Extern functions have no body to validate
    if (this->is_extern())
        return;

    if (!this->is_generic())
    {
        validate_function_overload(symbol_table, this);
        return;
    }

    //
    // For generic functions, we create a new copy of the function with all parameters resolved, and do validation
    // on that copy. This is because we want to validate the function body with the actual types that will be used in
    // the function, rather than the generic placeholders.
    //
    // create a copy of this function with the parameters instantiated
    for (const auto& node : this->get_generic_instantiations())
    {
        validate_function_overload(node->get_body()->get_symbol_table().get(), node.get());
    }
}

void IAstFunction::validate_function_overload(SymbolTable* symbol_table, IAstFunction* candidate_function)
{
    auto* annotated_candidate_return_ty = candidate_function->get_return_type();

    if (annotated_candidate_return_ty->is_alias_ty())
    {
        annotated_candidate_return_ty =
            cast_type<AstAliasType*>(annotated_candidate_return_ty)->get_primitive_base_type(symbol_table);
    }

    const auto return_statements = collect_return_statements(candidate_function->get_body());

    // For void types, we only disallow returning expressions, as this is redundant.
    if (const auto void_ret = cast_type<AstPrimitiveType*>(annotated_candidate_return_ty);
        void_ret != nullptr && void_ret->get_primitive_type() == PrimitiveType::VOID)
    {
        for (const auto& return_stmt : return_statements)
        {
            if (return_stmt->get_return_expression().has_value())
            {
                throw stride_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "{} has return type 'void' and cannot return a value.",
                        candidate_function->is_anonymous()
                        ? "Anonymous function"
                        : std::format("Function '{}'", candidate_function->get_function_name())),
                    {
                        ErrorSourceReference(
                            "unexpected return value",
                            return_stmt->get_source_position()
                        ),
                        ErrorSourceReference(
                            "Function returning void type",
                            candidate_function->get_source_position()
                        )
                    }

                );
            }
        }
        candidate_function->get_body()->validate(symbol_table);
        return;
    }

    if (return_statements.empty())
    {
        if (cast_type<AstAliasType*>(annotated_candidate_return_ty))
        {
            throw stride_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' returns a struct type, but no return statement is present.",
                    candidate_function->get_function_name()),
                candidate_function->get_source_position());
        }

        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "{} is missing a return statement.",
                candidate_function->is_anonymous()
                ? "Anonymous function"
                : std::format("Function '{}'", candidate_function->get_function_name())),
            candidate_function->get_source_position()
        );
    }

    for (const auto& return_stmt : return_statements)
    {
        if (return_stmt->is_void_type())
        {
            if (!annotated_candidate_return_ty->is_void_ty())
            {
                throw stride_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "Function '{}' returns a value of type '{}', but no return statement is present.",
                        candidate_function->is_anonymous() ? "<anonymous function>" : candidate_function->get_function_name(),
                        annotated_candidate_return_ty->get_type_name()),
                    return_stmt->get_source_position()
                );
            }
            return;
        }
        const auto& ret_expr = return_stmt->get_return_expression().value();

        IAstType* ret_expr_ty = ret_expr->get_type();

        if (ret_expr->get_type()->is_alias_ty())
        {
            ret_expr_ty = cast_type<AstAliasType*>(ret_expr->get_type())->get_primitive_base_type(symbol_table);
        }

        if (
            !ret_expr_ty->equals(symbol_table, annotated_candidate_return_ty) &&
            !ret_expr_ty->is_assignable_to(symbol_table, annotated_candidate_return_ty))
        {
            const auto error_fragment = ErrorSourceReference(
                std::format(
                    "expected {}{}",
                    candidate_function->get_return_type()->is_primitive()
                    ? ""
                    : candidate_function->get_return_type()->is_function()
                    ? "function-type "
                    : "struct-type ",
                    candidate_function->get_return_type()->get_type_name()),
                ret_expr->get_source_position()
            );

            throw stride_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' expected a return type of '{}', but received '{}'.",
                    candidate_function->is_anonymous() ? "<anonymous function>" : candidate_function->get_function_name(),
                    annotated_candidate_return_ty->get_type_name(),
                    ret_expr->get_type()->get_type_name()),
                { error_fragment }
            );
        }
    }

    candidate_function->get_body()->validate(symbol_table);
}

std::vector<AstReturnStatement*> IAstFunction::collect_return_statements(const AstBlock* body)
{
    if (!body)
    {
        return {};
    }

    std::vector<AstReturnStatement*> return_statements;
    for (const auto& child : body->get_children())
    {
        if (auto* return_stmt = dynamic_cast<AstReturnStatement*>(child.get()))
        {
            return_statements.push_back(return_stmt);
        }

        // Recursively collect from child containers
        if (const auto container_node = dynamic_cast<IAstContainer*>(child.
            get()))
        {
            const auto aggregated = collect_return_statements(
                container_node->get_body());
            return_statements.insert(return_statements.end(),
                                     aggregated.begin(),
                                     aggregated.end());
        }

        // Edge case: if statements hold the `else` block too, though this doesn't fall under the
        // `IAstContainer` abstraction. The `get_body` part is added in the previous case, though we
        // still need to add the else body
        if (const auto if_statement = dynamic_cast<AstConditionalStatement*>(child.
            get()))
        {
            const auto aggregated = collect_return_statements(
                if_statement->get_else_body());
            return_statements.insert(
                return_statements.end(),
                aggregated.begin(),
                aggregated.end()
            );
        }
    }
    return return_statements;
}
