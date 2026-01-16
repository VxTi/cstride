#include <iostream>
#include <fstream>
#include <sstream>

#include "program.h"

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
        const auto program = stride::Program({argv[1]});

        program.execute();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}