#include <algorithm>

#include "ast/scope.h"
#include "errors.h"

namespace stride::ast
{
    std::optional<SymbolDefinition> try_get_symbol_from_scope(
        const Scope* scope,
        const Symbol& symbol
    )
    {
        const auto end = scope->symbols->end();
        const auto it_result = std::ranges::find_if(
            scope->symbols->begin(), end,
            [&](const SymbolDefinition& s)
            {
                return s.get_symbol() == symbol;
            }
        );

        if (it_result == end)
        {
            return std::nullopt;
        }

        return *it_result;
    }

    std::optional<SymbolDefinition> Scope::get_symbol_globally(const Symbol& symbol) const
    {
        auto current = this;

        while (current != nullptr)
        {
            if (auto it = try_get_symbol_from_scope(current, symbol); it.has_value())
            {
                return it;
            }

            current = current->parent_scope.get();
        }

        return std::nullopt;
    }

    std::optional<SymbolDefinition> Scope::get_symbol(const Symbol& symbol) const
    {
        return try_get_symbol_from_scope(this, symbol);
    }

    bool Scope::is_symbol_defined_globally(const Symbol& symbol) const
    {
        const auto sym_definition = this->get_symbol_globally(symbol);

        return sym_definition.has_value();
    }

    bool Scope::is_symbol_defined_scoped(const Symbol& symbol) const
    {
        const auto sym_definition = this->get_symbol(symbol);

        return sym_definition.has_value();
    }

    void Scope::try_define_scoped_symbol(
        const SourceFile& source,
        const Token& token,
        const Symbol& symbol,
        const SymbolType symbol_type
    ) const
    {
        if (this->is_symbol_defined_scoped(symbol))
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
            SymbolDefinition(symbol, token, symbol_type)
        );
    }

    void Scope::try_define_global_symbol(
        const SourceFile& source,
        const Token& reference_token,
        const Symbol& symbol,
        const SymbolType symbol_type
    ) const
    {
        if (this->is_symbol_defined_globally(symbol))
        {
            throw parsing_error(
                make_source_error(
                    source,
                    ErrorType::SEMANTIC_ERROR,
                    "Symbol already defined globally",
                    reference_token.offset,
                    symbol.value.size()
                )
            );
        }
        this->symbols->push_back(
            SymbolDefinition(symbol, reference_token, symbol_type)
        );
    }
}
