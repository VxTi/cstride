#pragma once
#include <fstream>
#include <string>

namespace stride
{
    struct SourceFile
    {
        std::string path;
        std::string source;

        SourceFile(std::string path, std::string source)
            : path(std::move(path)), source(std::move(source))
        {
        }
    };

    std::shared_ptr<SourceFile> read_file(const std::string& path);
}
