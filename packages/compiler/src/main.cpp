#include "cli.h"

int main(const int argc, char* argv[])
{
    return stride::cli::resolve_cli_command(argc, argv);
}
