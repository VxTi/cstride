#pragma once

#include "files.h"

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
    class SymbolTable;
    class IAstType;
    class TokenSet;

    struct GenericParameterName
    {
        SourcePosition position;
        std::string name;
    };

    using GenericParameterList = std::vector<GenericParameterName>;
    using GenericTypeList = std::vector<std::unique_ptr<IAstType>>;

#define EMPTY_GENERIC_PARAMETER_LIST (GenericParameterList{})
#define EMPTY_GENERIC_TYPE_LIST (GenericTypeList{})

    GenericParameterList parse_generic_declaration(TokenSet& set);

    GenericTypeList parse_generic_type_arguments(TokenSet& set);

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
}
