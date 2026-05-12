#pragma once

#include "files.h"

#include <string>
#include <utility>
#include <vector>

#define MAIN_FN_NAME ("main")
#define DELIMITER ("__")

namespace stride::ast
{
    class IAstType;
    class SymbolTable;

    struct Symbol
    {
        /// Human-readable name of this symbol
        std::string name;

        /// Internalized name of this symbol.
        /// Can be the same as `name`, if there's no need for internalization.
        std::string internal_name;

        SourcePosition symbol_position;

        explicit Symbol(
            const SourcePosition& position,
            const std::string& context_name,
            std::string name,
            const std::string& internal_name
        ) :
            name(std::move(name)),
            internal_name(
                context_name.empty()
                ? internal_name
                : context_name + DELIMITER + internal_name),
            symbol_position(position) {}

        explicit Symbol(
            const SourcePosition& position,
            const std::string& context_name,
            const std::string& name
        ) :
            Symbol(position, context_name, name, name) {}

        explicit Symbol(
            const SourcePosition& position,
            const std::string& name
        ) :
            Symbol(position, "", name) {}

        bool operator==(const Symbol& other) const
        {
            return internal_name == other.internal_name;
        }
    };

    using SymbolNameSegments = std::vector<std::string>;

    Symbol resolve_internal_name(
        const std::string& context_name,
        const SourcePosition& position,
        const SymbolNameSegments& segments);

    std::string resolve_internal_name(const SymbolNameSegments& segments);
} // namespace stride::ast
