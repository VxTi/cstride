#include "ast/symbols.h"

#include "formatting.h"
#include "ast/symbol_table.h"

#include <ranges>

using namespace stride::ast;

Symbol stride::ast::resolve_internal_name(
    const std::string& context_name,
    const SourcePosition& position,
    const SymbolNameSegments& segments)
{
    return Symbol(position, context_name, resolve_internal_name(segments));
}

std::string stride::ast::resolve_internal_name(const SymbolNameSegments& segments)
{
    return join(segments, DELIMITER);
}
