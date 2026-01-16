#pragma once
#include <vector>
#include <string>

namespace stride::ast
{
    typedef struct Symbol
    {
        const std::string &value;

        bool operator==(const Symbol& other) const
        {
            return value == other.value;
        }

        static Symbol from_segments(const std::vector<std::string>& segments)
        {
            std::string initial = "";
            for (const auto& segment : segments)
            {
                initial += segment;
                if (&segment != &segments.back())
                {
                    initial += "_";
                }
            }
            return Symbol{ initial };
        }

        [[nodiscard]] std::string to_string() const { return value; }
    } Symbol;
}
