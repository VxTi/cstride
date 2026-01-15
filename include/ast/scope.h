#pragma once
#include <memory>
#include <string>

#include "files.h"
#include "symbol.h"

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


    class Scope;

    class Scope
    {
    public:
        ScopeType type;
        std::shared_ptr<Scope> parent_scope;
        std::unique_ptr<std::vector<Symbol>> symbols;

        Scope(
            std::shared_ptr<Scope>&& parent,
            const ScopeType type
        )
            : type(type),
              parent_scope(std::move(parent)),
              symbols(std::make_unique<std::vector<Symbol>>())
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

        void try_define_symbol(const SourceFile& source, size_t source_offset, const Symbol& symbol) const;

        [[nodiscard]] bool is_symbol_defined(const Symbol& symbol) const;
    };
}
