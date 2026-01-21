#include <sstream>

#include "files.h"
#include "errors.h"
#include "ast/nodes/ast_node.h"

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

    return std::make_shared<SourceFile>(
        path,
        std::move(content)
    );
}