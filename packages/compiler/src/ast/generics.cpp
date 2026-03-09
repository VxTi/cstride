#include "ast/generics.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
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

GenericTypeList stride::ast::parse_generic_arguments(const std::shared_ptr<ParsingContext>& context, TokenSet& set)
{
    GenericTypeList generic_params;
    if (set.peek_next_eq(TokenType::LT))
    {
        set.next();
        generic_params.push_back(
            parse_type(context, set, "Expected generic parameter name")
        );

        while (set.peek_next_eq(TokenType::COMMA))
        {
            set.next();
            generic_params.push_back(
                parse_type(context, set, "Expected generic parameter name")
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

    if (auto* array_type = cast_type<AstArrayType*>(type))
    {
        return std::make_unique<AstArrayType>(
            array_type->get_source_fragment(),
            array_type->get_context(),
            resolve_generics(array_type->get_element_type(), param_names, instantiated_types),
            array_type->get_initial_length(),
            array_type->get_flags()
        );
    }

    if (const auto* struct_type = cast_type<AstStructType*>(type))
    {
        const auto& members = struct_type->get_members();
        StructTypeMemberList resolved_members;
        resolved_members.reserve(members.size());

        for (const auto& [field_name, field_ty] : members)
        {
            resolved_members.emplace_back(
                field_name,
                resolve_generics(field_ty.get(), param_names, instantiated_types)
            );
        }

        return std::make_unique<AstStructType>(
            struct_type->get_source_fragment(),
            struct_type->get_context(),
            std::move(resolved_members),
            struct_type->get_flags()
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
    const AstAliasType* named_type,
    const definition::TypeDefinition* type_definition
)
{
    const auto& instantiated_types = named_type->get_instantiated_generic_types();
    const auto& generic_param_names = type_definition->get_generics_parameters();

    const auto& base_type = type_definition->get_type();

    // Ensure we instantiate the type with the correct amount of parameters
    if (instantiated_types.size() != generic_param_names.size())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Failed to instantiate generic type type '{}': expected {} parameters, got {}",
                named_type->get_name(),
                generic_param_names.size(),
                instantiated_types.size()
            ),
            named_type->get_source_fragment()
        );
    }

    return resolve_generics(base_type, generic_param_names, instantiated_types);
}
