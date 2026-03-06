#include "files.h"

#include "errors.h"

#include <sstream>

using namespace stride;

std::shared_ptr<SourceFile> stride::read_file(const std::string& path)
{
    const std::ifstream file(path);

    if (!file)
    {
        throw parsing_error("Failed to open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    auto content = buffer.str();

    return std::make_shared<SourceFile>(path, std::move(content));
}

SourceFragment SourceFragment::concat(const SourceFragment& first, const SourceFragment& last)
{
    return SourceFragment(
        first.source,
        first.offset,
        (last.offset + last.length) - first.offset
    );
}
