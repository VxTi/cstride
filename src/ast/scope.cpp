#include <algorithm>

#include "ast/scope.h"
#include "errors.h"

namespace stride::ast
{
    std::optional<SymbolDefinition> try_get_symbol_from_scope(
        const Scope* scope,
        const std::string& symbol
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

    std::optional<SymbolDefinition> Scope::get_symbol_globally(const std::string& symbol) const
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

    std::optional<SymbolDefinition> Scope::get_symbol(const std::string& symbol) const
    {
        return try_get_symbol_from_scope(this, symbol);
    }

    bool Scope::is_symbol_defined_globally(const std::string& symbol) const
    {
        const auto sym_definition = this->get_symbol_globally(symbol);

        return sym_definition.has_value();
    }

    bool Scope::is_symbol_defined_scoped(const std::string& symbol) const
    {
        const auto sym_definition = this->get_symbol(symbol);

        return sym_definition.has_value();
    }

    void Scope::try_define_scoped_symbol(
        const SourceFile& source,
        const Token& token,
        const std::string& symbol,
        const SymbolType symbol_type,
        const std::string& internal_name
    ) const
    {
        if (this->is_symbol_defined_scoped(internal_name))
        {
            throw parsing_error(
                make_source_error(
                    source,
                    ErrorType::SEMANTIC_ERROR,
                    "Symbol already defined in this scope",
                    token.offset,
                    symbol.size()
                )
            );
        }
        this->symbols->push_back(
            SymbolDefinition(symbol, token, symbol_type, internal_name)
        );
    }

    void Scope::try_define_global_symbol(
        const SourceFile& source,
        const Token& reference_token,
        const std::string& symbol,
        const SymbolType symbol_type,
        const std::string& internal_name
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
                    symbol.size()
                )
            );
        }
        this->symbols->push_back(
            SymbolDefinition(symbol, reference_token, symbol_type, internal_name)
        );
    }

    bool Scope::is_function_defined_globally(const std::string& internal_name) const
    {
        if (const auto it = try_get_symbol_from_scope(this, internal_name); it.has_value())
        {
            auto sym_def = it.value();
            if (const auto* fn_def = dynamic_cast<SymbolFnDefinition*>(&sym_def))
            {
                if (fn_def->get_internal_name() == internal_name)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void Scope::try_define_function_symbol(
        const SourceFile& source,
        const Token& reference_token,
        const std::string& symbol,
        const std::vector<unique_ptr<IAstInternalFieldType>>& parameter_types,
        const shared_ptr<IAstInternalFieldType>& return_type,
        bool anonymous = false
    )
    {
        if (!anonymous && this->type != ScopeType::GLOBAL)
        {
            throw parsing_error(
                make_source_error(
                    source,
                    ErrorType::SEMANTIC_ERROR,
                    "Only global scopes can define non-anonymous functions",
                    reference_token.offset,
                    symbol.size()
                )
            );
        }

        const auto internal_name = resolve_internal_function_name(parameter_types, symbol);
        if (this->is_function_defined_globally(internal_name))
        {
            throw parsing_error(
                make_source_error(
                    source,
                    ErrorType::SEMANTIC_ERROR,
                    "Function already defined in this scope",
                    reference_token.offset,
                    symbol.size()
                )
            );
        }

        this->symbols->push_back(
            std::make_unique<SymbolFnDefinition>(
                symbol,
                reference_token,
                std::move(parameter_types),
                return_type,
                internal_name
            )
        );
    }
}
