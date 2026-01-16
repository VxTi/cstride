#include "program.h"

#include "ast/parser.h"

using namespace stride;

Program::Program(std::vector<std::string> files)
{
    std::vector<ProgramObject> nodes;

    // Transform provided files into Program Objects (IR file representation)
    std::ranges::transform(
        files, std::back_inserter(nodes),
        [](const std::string& file) -> ProgramObject
        {
            return ProgramObject(ast::parser::parse(file));
        });

    this->_nodes = std::move(nodes);
}
