#include "ast/symbols.h"

#include "formatting.h"
#include "ast/parsing_context.h"

#include <ranges>

using namespace stride::ast;

/**
 * Function names will be internalized by their context name and parameter types.
 * This allows functions to be overloaded by their parameter types, as well as preventing name
 * clashes between different contexts.
 */
Symbol stride::ast::resolve_internal_function_name(
    const std::shared_ptr<ParsingContext>& context,
    const SourceFragment& position,
    const SymbolNameSegments& function_name_segments,
    const std::vector<IAstType*>& parameter_types)
{
    if (function_name_segments.size() == 1 && function_name_segments.at(0) ==
        MAIN_FN_NAME)
    {
        return Symbol(position, MAIN_FN_NAME);
    }

    std::string params;

    // Not perfect, but semi unique
    for (const auto& type : parameter_types)
    {
        params += type->to_string();
    }

    const auto function_name = resolve_internal_name(function_name_segments);

    return Symbol(
        position,
        context->get_name(),
        function_name,
        std::format("{}${:x}",
                    function_name,
                    std::hash<std::string>{}(params))
    );
}


Symbol stride::ast::resolve_internal_name(
    const std::string& context_name,
    const SourceFragment& position,
    const SymbolNameSegments& segments)
{
    return Symbol(position, context_name, resolve_internal_name(segments));
}

std::string stride::ast::resolve_internal_name(
    const SymbolNameSegments& segments)
{
    return join(segments, DELIMITER);
}
