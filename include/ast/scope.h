#pragma once
#include <memory>
#include <string>
#include <optional>

#include "files.h"
#include "symbol.h"
#include "tokens/token.h"

namespace stride::ast
{
    enum class ScopeType
    {
        GLOBAL,
        FUNCTION,
        CLASS,
        BLOCK
    };

    static std::string scope_type_to_str(const ScopeType& type)
    {
        switch (type)
        {
        case ScopeType::GLOBAL: return "global";
        case ScopeType::FUNCTION: return "function";
        case ScopeType::CLASS: return "class";
        case ScopeType::BLOCK: return "block";
        }
        return "unknown";
    }

    class SymbolDefinition
    {
    public:
        Symbol symbol;
        const Token& reference_token;

        explicit SymbolDefinition(
            const Symbol& symbol,
            const Token& reference_token
        ) : symbol(symbol),
            reference_token(reference_token)
        {
        }
    };


    class Scope;

    class Scope
    {
    public:
        ScopeType type;
        std::shared_ptr<Scope> parent_scope;
        std::unique_ptr<std::vector<SymbolDefinition>> symbols;

        Scope(
            std::shared_ptr<Scope>&& parent,
            const ScopeType type
        )
            : type(type),
              parent_scope(std::move(parent)),
              symbols(std::make_unique<std::vector<SymbolDefinition>>())
        {
        }

        explicit Scope(const ScopeType type)
            : Scope(nullptr, type)
        {
        }

        explicit Scope(const Scope& parent, const ScopeType scope)
            : Scope(
                std::shared_ptr<Scope>(const_cast<Scope*>(&parent), [](Scope*)
                {
                }),
                scope
            )
        {
        }

        std::optional<SymbolDefinition> try_get_symbol_globally(const Symbol& symbol) const;

        /**
         * Searches for a symbol only in the current scope, not in parent scopes.
         * @param symbol Symbol to search for
         * @return SymbolDefinition if found in the current scope, std::nullopt otherwise
         * @example auto def = scope.try_get_symbol(Symbol("local_var"));
         */
        std::optional<SymbolDefinition> try_get_symbol(const Symbol& symbol) const;

        /**
         * Defines a symbol in this scope after checking it doesn't exist globally.
         * @param source Source file for error reporting
         * @param token Token representing the symbol for error reporting
         * @param symbol Symbol to define
         * @throws parsing_error if the symbol already exists in this scope or any parent scope
         */
        void try_define_symbol(const SourceFile& source, const Token& token, const Symbol& symbol) const;

        /**
         * Defines a symbol in this scope after checking it doesn't exist in the current scope only.
         * @param source Source file for error reporting
         * @param token Token representing the symbol for error reporting
         * @param symbol Symbol to define
         * @throws parsing_error if the symbol already exists in this scope (ignores parent scopes)
         */
        void try_define_symbol_isolated(const SourceFile& source, const Token& token, const Symbol& symbol) const;

        /**
         * Checks if a symbol exists in this scope or any parent scope.
         * @param symbol Symbol to check
         * @return true if the symbol exists globally, false otherwise
         */
        bool is_symbol_defined_globally(const Symbol& symbol) const;

        /**
         * Checks if a symbol exists only in the current scope.
         * @param symbol Symbol to check
         * @return true if the symbol exists in the current scope, false otherwise
         */
        [[nodiscard]] bool is_symbol_defined_isolated(const Symbol& symbol) const;
    };
}
