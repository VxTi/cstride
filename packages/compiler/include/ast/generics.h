#pragma once
#include <memory>
#include <string>
#include <vector>

namespace stride::ast
{
    namespace definition
    {
        class TypeDefinition;
    }

    class AstAliasType;
    class ParsingContext;
    class IAstType;
    class TokenSet;

    using GenericParameter = std::string;
    using GenericParameterList = std::vector<GenericParameter>;
    using GenericTypeList = std::vector<std::unique_ptr<IAstType>>;

    GenericParameterList parse_generic_declaration(TokenSet& set);

    GenericTypeList parse_generic_arguments(const std::shared_ptr<ParsingContext>& context, TokenSet& set);

    std::unique_ptr<IAstType> resolve_generics(
        IAstType* type,
        const GenericParameterList& param_names,
        const GenericTypeList& instantiated_types
    );

    std::unique_ptr<IAstType> instantiate_generic_type(
        const AstAliasType* named_type,
        const definition::TypeDefinition* type_definition
    );
}
