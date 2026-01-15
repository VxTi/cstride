#include "../include/ast/tokens/tokenizer.h"
#include "../include/ast/parser.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace stride::ast;

int main(const int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: stride <source-file>" << std::endl;
        return 1;
    }

    std::cout << "Starting Stride compilation..." << std::endl;

    try
    {
        const auto result = parser::parse(argv[1]);

        const auto nodes = result->to_string();

        std::cout << "Stride compilation completed: " << nodes << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr  << e.what() << std::endl;
        return 1;
    }

    return 0;
}
