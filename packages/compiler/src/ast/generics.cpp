#include "ast/generics.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/type_inference.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/types.h"
#include "ast/nodes/while_loop.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <ranges>

using namespace stride::ast;

GenericParameterList stride::ast::parse_generic_declaration(TokenSet& set)
{
    GenericParameterList generic_params;
    if (set.peek_next_eq(TokenType::LT))
    {
        set.next();
        generic_params.push_back(
            set.expect(TokenType::IDENTIFIER, "Expected generic parameter name").get_lexeme()
        );

        while (set.peek_next_eq(TokenType::COMMA))
        {
            set.next();
            generic_params.push_back(
                set.expect(TokenType::IDENTIFIER, "Expected generic parameter name").get_lexeme()
            );
        }

        set.expect(TokenType::GT);
    }
    return generic_params;
}

GenericTypeList stride::ast::parse_generic_type_arguments(const std::shared_ptr<ParsingContext>& context, TokenSet& set)
{
    GenericTypeList generic_params;
    if (set.peek_next_eq(TokenType::LT))
    {
        set.next();
        generic_params.push_back(
            parse_type(context, set, { "Expected generic parameter name" })
        );

        while (set.peek_next_eq(TokenType::COMMA))
        {
            set.next();
            generic_params.push_back(
                parse_type(context, set, { "Expected generic parameter name" })
            );
        }

        set.expect(TokenType::GT);
    }
    return std::move(generic_params);
}

std::unique_ptr<IAstType> stride::ast::resolve_generics(
    IAstType* type,
    const GenericParameterList& param_names,
    const GenericTypeList& instantiated_types
)
{
    if (param_names.size() != instantiated_types.size())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Failed to resolve generic type: expected {} parameters, got {}",
                param_names.size(),
                instantiated_types.size()),
            type->get_source_fragment()
        );
    }
    if (auto* named_type = cast_type<AstAliasType*>(type))
    {
        for (size_t i = 0; i < param_names.size(); i++)
        {
            if (param_names[i] == named_type->get_name())
            {
                return instantiated_types[i]->clone_ty();
            }
        }

        if (named_type->is_generic_overload())
        {
            GenericTypeList resolved_params;
            for (const auto& param : named_type->get_instantiated_generic_types())
            {
                resolved_params.push_back(resolve_generics(param.get(), param_names, instantiated_types));
            }

            return std::make_unique<AstAliasType>(
                named_type->get_source_fragment(),
                named_type->get_context(),
                named_type->get_name(),
                named_type->get_flags(),
                std::move(resolved_params)
            );
        }

        return named_type->clone_ty();
    }

    if (const auto* array_type = cast_type<AstArrayType*>(type))
    {
        return std::make_unique<AstArrayType>(
            array_type->get_source_fragment(),
            array_type->get_context(),
            resolve_generics(array_type->get_element_type(), param_names, instantiated_types),
            array_type->get_initial_length(),
            array_type->get_flags()
        );
    }

    if (const auto* object_type = cast_type<AstObjectType*>(type))
    {
        const auto& members = object_type->get_members();
        ObjectTypeMemberList resolved_members;
        resolved_members.reserve(members.size());

        for (const auto& [field_name, field_ty] : members)
        {
            resolved_members.emplace_back(
                field_name,
                resolve_generics(field_ty.get(), param_names, instantiated_types)
            );
        }

        GenericTypeList resolved_generics;
        if (const auto& instantiated_generics = object_type->get_instantiated_generics();
            instantiated_generics.empty() && !instantiated_types.empty() && !param_names.empty())
        {
            resolved_generics.reserve(instantiated_types.size());
            for (const auto& gen : instantiated_types)
            {
                resolved_generics.push_back(gen->clone_ty());
            }
        }
        else
        {
            resolved_generics.reserve(instantiated_generics.size());
            for (const auto& gen : instantiated_generics)
            {
                resolved_generics.push_back(resolve_generics(gen.get(), param_names, instantiated_types));
            }
        }

        return std::make_unique<AstObjectType>(
            object_type->get_source_fragment(),
            object_type->get_context(),
            object_type->get_base_name(),
            std::move(resolved_members),
            object_type->get_flags(),
            std::move(resolved_generics)
        );
    }

    if (const auto* func_type = cast_type<AstFunctionType*>(type))
    {
        std::vector<std::unique_ptr<IAstType>> resolved_params;
        for (const auto& param : func_type->get_parameter_types())
        {
            resolved_params.push_back(resolve_generics(param.get(), param_names, instantiated_types));
        }

        return std::make_unique<AstFunctionType>(
            func_type->get_source_fragment(),
            func_type->get_context(),
            std::move(resolved_params),
            resolve_generics(func_type->get_return_type().get(), param_names, instantiated_types),
            EMPTY_GENERIC_PARAMETER_LIST,
            // No more generics; they're resolved.
            func_type->get_flags()
        );
    }

    if (const auto* tuple_type = cast_type<AstTupleType*>(type))
    {
        std::vector<std::unique_ptr<IAstType>> resolved_members;
        for (const auto& member : tuple_type->get_members())
        {
            resolved_members.push_back(resolve_generics(member.get(), param_names, instantiated_types));
        }

        return std::make_unique<AstTupleType>(
            tuple_type->get_source_fragment(),
            tuple_type->get_context(),
            std::move(resolved_members),
            tuple_type->get_flags()
        );
    }

    return type->clone_ty();
}

std::unique_ptr<IAstType> stride::ast::instantiate_generic_type(
    const AstAliasType* alias_type,
    const definition::TypeDefinition* type_definition)
{
    const auto& instantiated_types = alias_type->get_instantiated_generic_types();
    const auto& generic_param_names = type_definition->get_generics_parameters();

    const auto& base_type = type_definition->get_type();

    // Ensure we instantiate the type with the correct amount of parameters
    if (instantiated_types.size() != generic_param_names.size())
    {
        if (generic_param_names.empty())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Failed to resolve generic for type '{}': type is not generic",
                    alias_type->get_name()
                ),
                alias_type->get_source_fragment()
            );
        }
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Failed to instantiate generic type '{}': expected {} parameters, got {}",
                alias_type->get_name(),
                generic_param_names.size(),
                instantiated_types.size()
            ),
            alias_type->get_source_fragment()
        );
    }

    return resolve_generics(base_type, generic_param_names, instantiated_types);
}

std::unique_ptr<AstObjectType> stride::ast::instantiate_generic_type(
    const AstObjectInitializer* object,
    AstObjectType* type,
    const definition::TypeDefinition* type_definition
)
{
    const auto& generic_param_names = type_definition->get_generics_parameters();
    const auto& instantiated_types = object->get_generic_type_arguments();

    // If there's no generic parameters, we can just return the initially provided type.
    if (generic_param_names.empty() && instantiated_types.empty())
    {
        return type->clone_as<AstObjectType>();
    }

    if (generic_param_names.size() != instantiated_types.size())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Failed to instantiate generic type type '{}': expected {} parameters, got {}",
                type->to_string(),
                generic_param_names.size(),
                instantiated_types.size()
            ),
            object->get_source_fragment()
        );
    }

    auto resolved_members = ObjectTypeMemberList();
    resolved_members.reserve(type->get_members().size());

    for (const auto& [field_name, field_ty] : type->get_members())
    {
        resolved_members.emplace_back(
            field_name,
            resolve_generics(field_ty.get(), generic_param_names, instantiated_types)
        );
    }

    GenericTypeList resolved_args;
    resolved_args.reserve(instantiated_types.size());
    for (const auto& arg : instantiated_types)
    {
        resolved_args.push_back(arg->clone_ty());
    }

    return std::make_unique<AstObjectType>(
        type->get_source_fragment(),
        type->get_context(),
        type->get_base_name(),
        std::move(resolved_members),
        type->get_flags(),
        std::move(resolved_args)
    );
}

GenericTypeList stride::ast::copy_generic_type_list(const GenericTypeList& list)
{
    GenericTypeList copy;
    copy.reserve(list.size());
    for (const auto& type : list)
    {
        copy.push_back(type->clone_as<IAstType>());
    }
    return copy;
}

std::string stride::ast::get_overloaded_function_name(std::string function_name, const GenericTypeList& overload_types)
{
    if (overload_types.empty())
        return function_name;

    std::vector<std::string> generic_instantiation_type_names;
    generic_instantiation_type_names.reserve(overload_types.size());

    for (const auto& type : overload_types)
    {
        generic_instantiation_type_names.push_back(type->get_type_name());
    }

    return std::format(
        "{}${}",
        function_name,
        join(generic_instantiation_type_names, "_")
    );
}

/**
 * Resolves an expression's type by re-inferring it (from context) and then substituting
 * any generic parameter names with the concrete instantiated types.
 */
static void resolve_expression_type(
    stride::ast::IAstExpression* expr,
    const stride::ast::GenericParameterList& param_names,
    const stride::ast::GenericTypeList& instantiated_types
)
{
    auto inferred_type = stride::ast::infer_expression_type(expr);
    expr->set_type(
        stride::ast::resolve_generics(inferred_type.get(), param_names, instantiated_types)
    );
}

void stride::ast::resolve_generics_in_body(
    IAstNode* node,
    const GenericParameterList& param_names,
    const GenericTypeList& instantiated_types
)
{
    if (!node || param_names.empty())
        return;

    //
    // Statement / container nodes — recurse into children first (bottom-up).
    //

    if (auto* block = dynamic_cast<AstBlock*>(node))
    {
        for (const auto& child : block->get_children())
            resolve_generics_in_body(child.get(), param_names, instantiated_types);
        return;
    }

    if (auto* return_stmt = dynamic_cast<AstReturnStatement*>(node))
    {
        if (return_stmt->get_return_expression().has_value())
            resolve_generics_in_body(
                return_stmt->get_return_expression().value().get(),
                param_names, instantiated_types);
        return;
    }

    if (auto* conditional = dynamic_cast<AstConditionalStatement*>(node))
    {
        resolve_generics_in_body(conditional->get_condition(), param_names, instantiated_types);
        resolve_generics_in_body(conditional->get_body(), param_names, instantiated_types);
        if (conditional->get_else_body())
            resolve_generics_in_body(conditional->get_else_body(), param_names, instantiated_types);
        return;
    }

    if (auto* while_loop = dynamic_cast<AstWhileLoop*>(node))
    {
        if (while_loop->get_condition())
            resolve_generics_in_body(while_loop->get_condition(), param_names, instantiated_types);
        resolve_generics_in_body(while_loop->get_body(), param_names, instantiated_types);
        return;
    }

    if (auto* for_loop = dynamic_cast<AstForLoop*>(node))
    {
        if (for_loop->get_initializer())
            resolve_generics_in_body(for_loop->get_initializer(), param_names, instantiated_types);
        if (for_loop->get_condition())
            resolve_generics_in_body(for_loop->get_condition(), param_names, instantiated_types);
        if (for_loop->get_incrementor())
            resolve_generics_in_body(for_loop->get_incrementor(), param_names, instantiated_types);
        resolve_generics_in_body(for_loop->get_body(), param_names, instantiated_types);
        return;
    }

    //
    // Expression nodes — recurse into sub-expressions first, then resolve this node's type.
    //

    auto* expr = dynamic_cast<IAstExpression*>(node);
    if (!expr)
        return;

    // --- Recurse into child expressions (bottom-up order) ---

    if (auto* binary = cast_expr<IBinaryOp*>(expr))
    {
        resolve_generics_in_body(binary->get_left(), param_names, instantiated_types);
        resolve_generics_in_body(binary->get_right(), param_names, instantiated_types);
    }
    else if (auto* unary = cast_expr<AstUnaryOp*>(expr))
    {
        resolve_generics_in_body(&unary->get_operand(), param_names, instantiated_types);
    }
    else if (auto* var_decl = cast_expr<AstVariableDeclaration*>(expr))
    {
        if (var_decl->get_initial_value())
            resolve_generics_in_body(var_decl->get_initial_value(), param_names, instantiated_types);
    }
    else if (auto* fn_call = cast_expr<AstFunctionCall*>(expr))
    {
        for (const auto& arg : fn_call->get_arguments())
            resolve_generics_in_body(arg.get(), param_names, instantiated_types);
    }
    else if (auto* array = cast_expr<AstArray*>(expr))
    {
        for (const auto& elem : array->get_elements())
            resolve_generics_in_body(elem.get(), param_names, instantiated_types);
    }
    else if (auto* array_accessor = cast_expr<AstArrayMemberAccessor*>(expr))
    {
        resolve_generics_in_body(array_accessor->get_array_base(), param_names, instantiated_types);
        resolve_generics_in_body(array_accessor->get_index(), param_names, instantiated_types);
    }
    else if (auto* struct_init = cast_expr<AstObjectInitializer*>(expr))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
            resolve_generics_in_body(val.get(), param_names, instantiated_types);
    }
    else if (auto* tuple_init = cast_expr<AstTupleInitializer*>(expr))
    {
        for (const auto& member : tuple_init->get_members())
            resolve_generics_in_body(member.get(), param_names, instantiated_types);
    }
    else if (auto* reassign = cast_expr<AstVariableReassignment*>(expr))
    {
        resolve_generics_in_body(reassign->get_identifier(), param_names, instantiated_types);
        resolve_generics_in_body(reassign->get_value(), param_names, instantiated_types);
    }
    else if (auto* chained = cast_expr<AstChainedExpression*>(expr))
    {
        resolve_generics_in_body(chained->get_base(), param_names, instantiated_types);
    }
    else if (auto* type_cast = cast_expr<AstTypeCastOp*>(expr))
    {
        resolve_generics_in_body(type_cast->get_value(), param_names, instantiated_types);
    }
    else if (auto* indirect_call = cast_expr<AstIndirectCall*>(expr))
    {
        for (const auto& arg : indirect_call->get_args())
            resolve_generics_in_body(arg.get(), param_names, instantiated_types);
        resolve_generics_in_body(indirect_call->get_callee(), param_names, instantiated_types);
    }
    else if (auto* function_node = cast_expr<IAstFunction*>(expr))
    {
        resolve_generics_in_body(function_node->get_body(), param_names, instantiated_types);
    }

    // --- Now resolve the type for this expression node ---
    resolve_expression_type(expr, param_names, instantiated_types);
}
