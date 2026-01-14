#pragma once

namespace stride::ast
{
    enum class ScopeType
    {
        GLOBAL,
        FUNCTION,
        CLASS,
        BLOCK
    };

    class Scope
    {
    public:
        const ScopeType type;

        explicit Scope(const ScopeType type) : type(type)
        {
        }

        ~Scope() = default;
    };
}
