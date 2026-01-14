#include "ast/symbol.h"

std::vector<stride::ast::Symbol> stride::ast::symbol_registry = {};

bool stride::ast::is_symbol_defined(const Symbol& symbol)
{
    const auto element = std::find(
        symbol_registry.begin(),
        symbol_registry.end(),
        symbol
    );

    return element != symbol_registry.end();
}

void stride::ast::define_symbol(const Symbol& symbol)
{
    symbol_registry.push_back(symbol);
}
