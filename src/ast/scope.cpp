#include <algorithm>

#include "ast/scope.h"
#include "errors.h"

namespace stride::ast
{
    std::optional<SymbolDefinition> Scope::try_get_symbol(const Symbol& symbol) const
    {
        const auto it = std::ranges::find_if(this->symbols->begin(), this->symbols->end(),
                                             [&](const SymbolDefinition& s)
                                             {
                                                 return s.symbol.value == symbol.value;
                                             });

        if (it == this->symbols->end())
        {
            return std::nullopt;
        }

        return *it;
    }

    bool Scope::is_symbol_defined(const Symbol& symbol) const
    {
        const auto sym_definition = this->try_get_symbol(symbol);

        return sym_definition.has_value();
    }

    void Scope::try_define_symbol(const SourceFile& source, const Token& token, const Symbol& symbol) const
    {
        if (this->is_symbol_defined(symbol))
        {
            throw parsing_error(
                make_source_error(
                    source,
                    ErrorType::SEMANTIC_ERROR,
                    "Symbol already defined in this scope",
                    token.offset,
                    symbol.value.size()
                )
            );
        }
        this->symbols->push_back(
            SymbolDefinition(symbol, token)
        );
    }
}
