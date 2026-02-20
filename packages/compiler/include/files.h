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

    struct SourceLocation
    {
        size_t offset;
        size_t length;

        const std::shared_ptr<SourceFile> source;

        SourceLocation(
            const std::shared_ptr<SourceFile>& source,
            const size_t offset,
            const size_t length) :
            offset(offset),
            length(length),
            source(source) {}

        SourceLocation operator=(const SourceLocation& other)
        {
            if (this != &other)
            {
                offset = other.offset;
                length = other.length;
            }
            return *this;
        }
    };

    std::shared_ptr<SourceFile> read_file(const std::string& path);
} // namespace stride
