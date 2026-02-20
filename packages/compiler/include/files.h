#pragma once
#include <fstream>
#include <memory>
#include <string>

namespace stride
{
    struct SourceFile
    {
        std::string path;
        std::string source;

        SourceFile(std::string path, std::string source) :
            path(std::move(path)),
            source(std::move(source)) {}
    };

    struct SourceFragment
    {
        size_t offset;
        size_t length;

        std::shared_ptr<SourceFile> source;

        SourceFragment(
            const std::shared_ptr<SourceFile>& source,
            const size_t offset,
            const size_t length) :
            offset(offset),
            length(length),
            source(source) {}

        SourceFragment& operator=(const SourceFragment& other)
        {
            if (this != &other)
            {
                offset = other.offset;
                length = other.length;
                source = other.source;
            }
            return *this;
        }
    };

    std::shared_ptr<SourceFile> read_file(const std::string& path);
} // namespace stride
