#pragma once
#include <string>
#include <vector>

namespace stride
{
    inline std::string join(const std::vector<std::string>& v,
                            const std::string& delimiter = ",")
    {
        std::string out;
        if (auto i = v.begin(), e = v.end(); i != e)
        {
            out += *i++;
            for (; i != e; ++i)
                out.append(delimiter).append(*i);
        }
        return out;
    }
} // namespace stride
