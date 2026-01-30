#include "cli.h"

#include <format>
#include <iostream>

#include "program.h"

using namespace stride::cli;

std::string format_message(std::string text)
{
    return std::format(
        "\x1b[48;2;0;102;204m\x1b[38;2;255;255;255mstride\x1b[0m"
        " \x1b[94m■\x1b[0m"
        " \x1b[0m{}\n",
        text
    );
}

int stride::cli::resolve_cli_command(const int argc, char** argv)
{
    // The first argument is always the command itself,
    if (argc < 2)
    {
        std::cout << format_message("No command provided. Usage: cstride <command> [options]");
        return 1;
    }

    const auto& command = std::string(argv[1]);

    if (command == "-h" || command == "--help")
    {
        std::cout << format_message("Usage: cstride <command> [options]\n");
        std::cout << "Available commands:" << std::endl;
        std::cout << "  -c, --compile <file1> <file2> ...    Compile stride files" << std::endl;
        std::cout << "  install, i <dependency>@<version>    Install the stride compiler" << std::endl;
        std::cout << "  -tc, --typecheck <file1> <file2> ... Typecheck a file" << std::endl;
        return 0;
    }

    if (command == "-c" || command == "--compile")
    {
        return resolve_compile_command(argc - 1, argv + 1);
    }

    if (command == "install" || command == "i")
    {
        return resolve_install_command(argc - 1, argv + 1);
    }
    if (command == "--typecheck" || command == "-tc")
    {
        return resolve_typecheck_command(argc - 1, argv + 1);
    }

    std::cout << format_message(std::format("Unknown command '{}'", command));

    return 1;
}

CompilationOptions resolve_compilation_options(const int argc, char** argv)
{
    CompilationOptions options = {
        .mode = CompilationMode::COMPILE_JIT,
        .debug_mode = false
    };

    for (int i = 0; i < argc; ++i)
    {
        const auto argument = std::string(argv[i]);
        if (!argument.starts_with("-"))
        {
            options.include_paths.emplace_back(argv[i]);

            continue;
        }

        // Currently, this doesn't really do anything else, as we don't support
        // interpreting yet.
        if (argument.starts_with("--mode="))
        {
            const auto mode = argument.substr(7);
            options.mode =
                mode == "interpret"
                    ? CompilationMode::INTERPRET
                    : CompilationMode::COMPILE_JIT;
        }

        if (argument == "--debug")
        {
            options.debug_mode = true;
        }

        // Handle other flags here in the future
    }

    return options;
}

// `cstride -c <...>` or `cstride --compile <...>`
int stride::cli::resolve_compile_command(const int argc, char** argv)
{
    const auto options = resolve_compilation_options(argc, argv);

    Program program;

    program.parse_files(options.include_paths);

    return program.compile_jit(options);
}

// `cstride install` or `cstride i`
int stride::cli::resolve_install_command(int argc, char** argv)
{
    // TODO: implemente
    return 0;
}

/// `cstride --typecheck` or `cstride -tc`
int stride::cli::resolve_typecheck_command(int argc, char** argv)
{
    // TODO: implement
    return 0;
}
