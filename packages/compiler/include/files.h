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
            : path(std::move(path)), source(std::move(source)) {}
    };

    struct SourcePosition
    {
        size_t offset;
        size_t length;

        SourcePosition(const size_t offset, const size_t length)
            : offset(offset), length(length) {}
    };

    std::shared_ptr<SourceFile> read_file(const std::string& path);
}
