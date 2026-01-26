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

    try
    {
        std::vector<std::string> files;
        for (int i = 1; i < argc; ++i)
        {
            files.emplace_back(argv[i]);
        }

        stride::Program program;
        program.parse_files(files);

        return program.compile_jit();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}