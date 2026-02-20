#pragma once
#include "nodes/types.h"

#include <string>
#include <utility>

namespace stride::ast
{
#define MAIN_FN_NAME ("main")
#define DELIMITER ("__")

    struct Symbol
    {
        /// Human-readable name of this symbol
        std::string name;

        /// Internalized name of this symbol.
        /// Can be the same as `name`, if there's no need for internalization.
        std::string internal_name;

        SourceFragment symbol_position;

        explicit Symbol(
            const SourceFragment& position,
            const std::string& context_name,
            std::string name,
            const std::string& internal_name) :
            name(std::move(name)),
            internal_name(
                context_name.empty()
                ? internal_name
                : context_name + DELIMITER + internal_name),
            symbol_position(position) {}

        explicit Symbol(
            const SourceFragment& position,
            const std::string& context_name,
            const std::string& name) :
            Symbol(position, context_name, name, name) {}

        explicit Symbol(const SourceFragment& position,
                        const std::string& name) :
            Symbol(position, "", name) {}

        bool operator==(const Symbol& other) const
        {
            return internal_name == other.internal_name;
        }
    };

    using SymbolNameSegments = std::vector<std::string>;

    Symbol resolve_internal_function_name(
        const std::shared_ptr<ParsingContext>& context,
        const SourceFragment& position,
        const SymbolNameSegments& function_name_segments,
        const std::vector<IAstType*>& parameter_types);

    Symbol resolve_internal_name(
        const std::string& context_name,
        const SourceFragment& position,
        const SymbolNameSegments& segments);

    std::string resolve_internal_name(const SymbolNameSegments& segments);
} // namespace stride::ast
