#include "ast/casting.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/return_statement.h"

using namespace stride::ast;

void IAstFunction::validate()
{
    // Extern functions have no body to validate
    if (this->is_extern())
        return;

    if (!this->is_generic())
    {
        validate_candidate(this);
        return;
    }

    //
    // For generic functions, we create a new copy of the function with all parameters resolved, and do validation
    // on that copy. This is because we want to validate the function body with the actual types that will be used in
    // the function, rather than the generic placeholders.
    //
    // create a copy of this function with the parameters instantiated
    for (const auto definition = this->get_function_definition();
         const auto& [instantiated_generic_types, function, node] : definition->get_generic_overloads())
    {
        auto instantiated_return_ty = resolve_generics(
            this->_annotated_return_type.get(),
            this->_generic_parameters,
            instantiated_generic_types
        );

        std::vector<std::unique_ptr<AstFunctionParameter>> instantiated_function_params;
        instantiated_function_params.reserve(this->_parameters.size());

        for (const auto& param : this->_parameters)
        {
            instantiated_function_params.push_back(
                std::make_unique<AstFunctionParameter>(
                    param->get_source_fragment(),
                    param->get_context(),
                    param->get_name(),
                    resolve_generics(param->get_type(), this->_generic_parameters, instantiated_generic_types)
                )
            );
        }

        node = std::make_unique<AstFunctionDeclaration>(
            this->get_context(),
            this->_symbol,
            std::move(instantiated_function_params),
            this->_body->clone_as<AstBlock>(),
            std::move(instantiated_return_ty),
            this->get_visibility(),
            this->_flags,
            this->get_generic_parameters()
        );

        validate_candidate(node.get());
    }
}

void IAstFunction::validate_candidate(IAstFunction* candidate)
{
    candidate->get_body()->validate();

    const auto& ret_ty = candidate->get_return_type();
    const auto return_statements = collect_return_statements(candidate->get_body());

    // For void types, we only disallow returning expressions, as this is redundant.
    if (const auto void_ret = cast_type<AstPrimitiveType*>(ret_ty);
        void_ret != nullptr && void_ret->get_primitive_type() == PrimitiveType::VOID)
    {
        for (const auto& return_stmt : return_statements)
        {
            if (return_stmt->get_return_expression().has_value())
            {
                throw parsing_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "{} has return type 'void' and cannot return a value.",
                        candidate->is_anonymous()
                        ? "Anonymous function"
                        : std::format("Function '{}'", candidate->get_plain_function_name())),
                    {
                        ErrorSourceReference(
                            "unexpected return value",
                            return_stmt->get_source_fragment()
                        ),
                        ErrorSourceReference(
                            "Function returning void type",
                            candidate->get_source_fragment()
                        )
                    }

                );
            }
        }
        return;
    }

    if (return_statements.empty())
    {
        if (cast_type<AstAliasType*>(ret_ty))
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' returns a struct type, but no return statement is present.",
                    candidate->get_plain_function_name()),
                candidate->get_source_fragment());
        }

        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "{} is missing a return statement.",
                candidate->is_anonymous()
                ? "Anonymous function"
                : std::format("Function '{}'", candidate->get_plain_function_name())),
            candidate->get_source_fragment()
        );
    }

    for (const auto& return_stmt : return_statements)
    {
        if (return_stmt->is_void_type())
        {
            if (!ret_ty->is_void_ty())
            {
                throw parsing_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "Function '{}' returns a value of type '{}', but no return statement is present.",
                        candidate->is_anonymous() ? "<anonymous function>" : candidate->get_plain_function_name(),
                        ret_ty->to_string()),
                    return_stmt->get_source_fragment()
                );
            }
            return;
        }

        if (const auto& ret_expr = return_stmt->get_return_expression().value();
            !ret_expr->get_type()->equals(ret_ty) &&
            !ret_expr->get_type()->is_assignable_to(ret_ty))
        {
            const auto error_fragment = ErrorSourceReference(
                std::format(
                    "expected {}{}",
                    candidate->get_return_type()->is_primitive()
                    ? ""
                    : candidate->get_return_type()->is_function()
                    ? "function-type "
                    : "struct-type ",
                    candidate->get_return_type()->to_string()),
                ret_expr->get_source_fragment()
            );

            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' expected a return type of '{}', but received '{}'.",
                    candidate->is_anonymous() ? "<anonymous function>" : candidate->get_plain_function_name(),
                    ret_ty->get_type_name(),
                    ret_expr->get_type()->get_type_name()),
                { error_fragment }
            );
        }
    }
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
