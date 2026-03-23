#include "files.h"

#include "errors.h"

#include <sstream>

using namespace stride;

std::unique_ptr<SourceFile> stride::read_file(const std::string& path)
{
    const std::ifstream file(path);

    if (!file)
    {
        throw stride_error("Failed to open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    auto content = buffer.str();

    return std::make_unique<SourceFile>(path, std::move(content));
}

SourcePosition SourcePosition::join(const SourcePosition& first, const SourcePosition& second)
{
    return {
        first.offset,
        (second.offset + second.length) - first.offset
    };
}
