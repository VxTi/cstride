#include "ast/casting.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/return_statement.h"

using namespace stride::ast;

void IAstFunction::validate(SymbolTable* symbol_table)
{
    // Extern functions have no body to validate
    if (this->is_extern())
        return;

    if (!this->is_generic())
    {
        validate_candidate(symbol_table, this);
        return;
    }

    //
    // For generic functions, we create a new copy of the function with all parameters resolved, and do validation
    // on that copy. This is because we want to validate the function body with the actual types that will be used in
    // the function, rather than the generic placeholders.
    //
    // create a copy of this function with the parameters instantiated
    for (const auto definition = this->get_function_definition(symbol_table);
         const auto& [instantiated_generic_types, function, node] : definition->get_generic_overloads())
    {
        auto instantiated_return_ty = resolve_generics(
            this->_annotated_return_type.get(),
            this->_generic_parameters,
            instantiated_generic_types
        );

        std::vector<std::unique_ptr<AstFunctionParameter>> instantiated_function_params;
        instantiated_function_params.reserve(this->_parameters.size());

        // Temporarily update parameter types in the context so resolve_generics_in_body
        // and subsequent validation can find the concrete types.
        std::vector<std::pair<definition::FieldDefinition*, std::unique_ptr<IAstType>>> old_param_types;
        for (const auto& param : this->_parameters)
        {
            if (auto def = symbol_table->lookup_variable(param->get_name(), true))
            {
                old_param_types.emplace_back(def, def->get_type()->clone());
                def->set_type(resolve_generics(def->get_type(), this->_generic_parameters, instantiated_generic_types));
            }

            instantiated_function_params.push_back(
                std::make_unique<AstFunctionParameter>(
                    param->get_source_position(),
                    param->get_name(),
                    resolve_generics(param->get_type(), this->_generic_parameters, instantiated_generic_types)
                )
            );
        }

        // Clone the body and resolve generic types on every expression within it.

        node = std::make_unique<AstFunctionDeclaration>(
            this->get_source_position(),
            this->get_function_name(),
            std::move(instantiated_function_params),
            this->_body->clone_as<AstBlock>(),
            std::move(instantiated_return_ty),
            this->get_visibility(),
            this->_flags,
            EMPTY_GENERIC_PARAMETER_LIST // Omit generics - They've been resolved
        );

        validate_candidate(symbol_table, node.get());

        // Restore original parameter types in the context
        for (auto& [def, old_type] : old_param_types)
        {
            def->set_type(std::move(old_type));
        }
    }
}

void IAstFunction::validate_candidate(SymbolTable* symbol_table, IAstFunction* candidate)
{

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
                throw stride_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "{} has return type 'void' and cannot return a value.",
                        candidate->is_anonymous()
                        ? "Anonymous function"
                        : std::format("Function '{}'", candidate->get_function_name())),
                    {
                        ErrorSourceReference(
                            "unexpected return value",
                            return_stmt->get_source_position()
                        ),
                        ErrorSourceReference(
                            "Function returning void type",
                            candidate->get_source_position()
                        )
                    }

                );
            }
        }
        candidate->get_body()->validate(symbol_table);
        return;
    }

    if (return_statements.empty())
    {
        if (cast_type<AstAliasType*>(ret_ty))
        {
            throw stride_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' returns a struct type, but no return statement is present.",
                    candidate->get_function_name()),
                candidate->get_source_position());
        }

        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "{} is missing a return statement.",
                candidate->is_anonymous()
                ? "Anonymous function"
                : std::format("Function '{}'", candidate->get_function_name())),
            candidate->get_source_position()
        );
    }

    for (const auto& return_stmt : return_statements)
    {
        if (return_stmt->is_void_type())
        {
            if (!ret_ty->is_void_ty())
            {
                throw stride_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "Function '{}' returns a value of type '{}', but no return statement is present.",
                        candidate->is_anonymous() ? "<anonymous function>" : candidate->get_function_name(),
                        ret_ty->get_type_name()),
                    return_stmt->get_source_position()
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
                    candidate->get_return_type()->get_type_name()),
                ret_expr->get_source_position()
            );

            throw stride_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' expected a return type of '{}', but received '{}'.",
                    candidate->is_anonymous() ? "<anonymous function>" : candidate->get_function_name(),
                    ret_ty->get_type_name(),
                    ret_expr->get_type()->get_type_name()),
                { error_fragment }
            );
        }
    }

    candidate->get_body()->validate(symbol_table);
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
