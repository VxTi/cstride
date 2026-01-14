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

        static Symbol from_segments(const std::vector<std::string>& segments)
        {
            Symbol symbol;
            symbol.value = "";
            for (size_t i = 0; i < segments.size(); i++)
            {
                symbol.value += segments[i];
                if (i < segments.size() - 1)
                {
                    symbol.value += "_";
                }
            }
            return symbol;
        }

        std::string to_string() const { return value; }
    } Symbol;

    extern std::vector<Symbol> symbol_registry;

    bool is_symbol_defined(const Symbol& symbol);

    void define_symbol(const Symbol& symbol);
}
