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
        const Symbol& symbol;
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

        std::optional<SymbolDefinition> try_get_symbol(const Symbol& symbol) const;

        void try_define_symbol(const SourceFile & source, const Token & token, const Symbol& symbol) const;

        [[nodiscard]] bool is_symbol_defined(const Symbol& symbol) const;
    };
}
