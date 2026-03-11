#include "ast/generics.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

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

    if (auto* object_type = cast_type<AstObjectType*>(type))
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
            object_type->get_type_name(),
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
    const definition::TypeDefinition* type_definition
)
{
    const auto& instantiated_types = alias_type->get_instantiated_generic_types();
    const auto& generic_param_names = type_definition->get_generics_parameters();

    const auto& base_type = type_definition->get_type();

    // Ensure we instantiate the type with the correct amount of parameters
    if (instantiated_types.size() != generic_param_names.size())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Failed to instantiate generic type type '{}': expected {} parameters, got {}",
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
        type->get_type_name(),
        std::move(resolved_members),
        type->get_flags(),
        std::move(resolved_args)
    );
}
