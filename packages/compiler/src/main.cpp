#include "cli.h"

#include <exception>
#include <iostream>

int main(const int argc, char* argv[])
{
    try
    {
        return stride::cli::resolve_cli_command(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
