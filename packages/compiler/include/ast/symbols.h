#pragma once
#include <string>
#include <utility>

#include "formatting.h"
#include "nodes/types.h"

namespace stride::ast
{
#define MAIN_FN_NAME ("main")

    struct Symbol
    {
        /// Human-readable name of this symbol
        std::string name;

        /// Internalized name of this symbol.
        /// Can be the same as `name`, if there's no need for internalization.
        std::string internal_name;

        explicit Symbol(
            std::string context_name,
            std::string name,
            std::string internal_name
        ) : name(std::move(name)),
            internal_name(join({std::move(context_name), std::move(internal_name)}, "_")) {}

        explicit Symbol(std::string context_name, const std::string& name) : Symbol(
            std::move(context_name), name, name) {}

        explicit Symbol(const std::string& name) : Symbol("", name) {}

        bool operator==(const Symbol& other) const
        {
            return internal_name == other.internal_name;
        }
    };

    /**
     * Will return a unique name for the provided function name, depending on its
     * parameter types. This way, one can construct a function with the same name,
     * but with different parameters types (function overloading)
     */
    Symbol resolve_internal_function_name(
        const std::vector<IAstType*>& parameter_types,
        const std::string& function_name
    );
}
