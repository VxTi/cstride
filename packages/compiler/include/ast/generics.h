#pragma once

#include <memory>
#include <string>
#include <vector>

namespace stride::ast
{
    class IAstNode;
    class AstObjectInitializer;
    class AstObjectType;

    namespace definition
    {
        class TypeDefinition;
    }

    class AstAliasType;
    class ParsingContext;
    class IAstType;
    class TokenSet;

    using GenericParameterList = std::vector<std::string>;
    using GenericTypeList = std::vector<std::unique_ptr<IAstType>>;

#define EMPTY_GENERIC_PARAMETER_LIST (GenericParameterList{})
#define EMPTY_GENERIC_TYPE_LIST (GenericTypeList{})

    GenericParameterList parse_generic_declaration(TokenSet& set);

    GenericTypeList parse_generic_type_arguments(const std::shared_ptr<ParsingContext>& context, TokenSet& set);

    std::unique_ptr<IAstType> resolve_generics(
        IAstType* type,
        const GenericParameterList& param_names,
        const GenericTypeList& instantiated_types
    );

    std::unique_ptr<IAstType> instantiate_generic_type(
        const AstAliasType* alias_type,
        const definition::TypeDefinition* type_definition
    );

    std::unique_ptr<AstObjectType> instantiate_generic_type(
        const AstObjectInitializer* object,
        AstObjectType* type,
        const definition::TypeDefinition* type_definition
    );

    GenericTypeList copy_generic_type_list(const GenericTypeList& list);

    std::string get_overloaded_function_name(std::string function_name, const GenericTypeList& overload_types);

    /**
     * Recursively walks a function body AST and resolves generic type parameters
     * on all expression nodes, in the same manner as resolve_generics does for types.
     *
     * For each expression, the type is re-inferred (via context lookup) and then
     * any generic parameters are substituted with their concrete instantiated types.
     * The walk is bottom-up so that child expression types are resolved before their parents.
     */
    void resolve_generics_in_body(
        IAstNode* node,
        const GenericParameterList& param_names,
        const GenericTypeList& instantiated_types
    );
}
