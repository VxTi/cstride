#pragma once
#include <vector>
#include <string>

namespace stride::ast
{
#define SEGMENT_DELIMITER ("__")

    static std::string internal_identifier_from_segments(const std::vector<std::string>& segments)
    {
        // Build a single string with '_' separators and return it by value.
        std::string initial;
        for (const auto& segment : segments)
        {
            initial += segment;
            if (&segment != &segments.back())
            {
                initial += SEGMENT_DELIMITER;
            }
        }
        return std::move(initial);
    }
}
