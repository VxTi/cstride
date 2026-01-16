#pragma once
#include <vector>
#include <string>

namespace stride::ast
{
#define MODULE_DELIMITER ("__")

    typedef struct Symbol
    {
        // Previously: const std::string &value;
        // Store an owning string to avoid dangling references when constructed from temporaries.
        std::string value;

        bool operator==(const Symbol& other) const
        {
            return value == other.value;
        }

        static Symbol from_segments(const std::vector<std::string>& segments)
        {
            // Build a single string with '_' separators and return it by value.
            std::string initial;
            for (const auto& segment : segments)
            {
                initial += segment;
                if (&segment != &segments.back())
                {
                    initial += MODULE_DELIMITER;
                }
            }
            return Symbol{ std::move(initial) };
        }

        [[nodiscard]] std::string to_string() const { return value; }
    } Symbol;
}
