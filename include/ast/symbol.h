#pragma once
#include <vector>
#include <string>

namespace stride::ast
{

    typedef struct Symbol
    {
        std::string value;

        bool operator==(const Symbol& other) const
        {
            return value == other.value;
        }
    } Symbol;

    extern std::vector<Symbol> symbol_registry;

    bool is_symbol_defined(const Symbol& symbol);

    void define_symbol(const Symbol& symbol);
}
