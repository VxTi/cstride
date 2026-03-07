#pragma once
#include <memory>
#include <string>
#include <vector>

namespace stride::ast
{
    class ParsingContext;
    class IAstType;
    class TokenSet;

    using GenericParameter = std::string;
    using GenericParameterList = std::vector<GenericParameter>;
    using GenericTypeList = std::vector<std::unique_ptr<IAstType>>;

    GenericParameterList parse_generic_declaration(TokenSet& set);

    GenericTypeList parse_generic_arguments(const std::shared_ptr<ParsingContext>& context, TokenSet& set);
}
