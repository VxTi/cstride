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
            {
                if (!out.empty())
                {
                    out.append(delimiter).append(*i);
                }
                else
                {
                    out.append(*i);
                }
            }
        }
        return out;
    }
} // namespace stride
