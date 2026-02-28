#include "cli.h"

#include "program.h"

#include <format>
#include <iostream>

using namespace stride::cli;

std::string format_message(std::string text)
{
    return std::format(
        "\x1b[48;2;0;102;204m\x1b[38;2;255;255;255mstride\x1b[0m"
        " \x1b[94m■\x1b[0m"
        " \x1b[0m{}\n",
        text);
}

int stride::cli::resolve_cli_command(const int argc, char** argv)
{
    // The first argument is always the command itself,
    const auto& command = argc > 1 ? std::string(argv[1]) : "";

    if (argc < 2 || command == "-h" || command == "--help" || command == "help")
    {
        std::cout << format_message("Usage: cstride <command> [options]\n");
        std::cout << "Available commands:" << std::endl;
        std::cout <<
            "  -c, --compile <file1> <file2> ...    Compile stride files" <<
            std::endl;
        std::cout <<
            "  -r, --run <file1> <file2> ...        Run stride files using JIT" <<
            std::endl;
        return 0;
    }

    if (command == "-c" || command == "--compile")
    {
        return resolve_compile_command(argc - 1, argv + 1);
    }

    if (command == "-r" || command == "--run")
    {
        return resolve_run_command(argc - 1, argv + 1);
    }

    std::cout << format_message(std::format("Unknown command '{}'", command));

    return 1;
}

CompilationOptions stride::cli::resolve_compilation_options_from_args(const int argc, char** argv)
{
    CompilationOptions options = { .mode       = CompilationMode::COMPILE_JIT,
                                   .debug_mode = false };

    for (int i = 0; i < argc; ++i)
    {
        const auto argument = std::string(argv[i]);
        if (!argument.starts_with("-"))
        {
            options.source_files.emplace_back(argv[i]);

            continue;
        }

        if (argument == "--output" || argument == "-o")
        {
            if (i + 1 < argc)
            {
                options.program_name = std::string(argv[++i]);
            }
            continue;
        }

        if (argument == "--dir" || argument == "-d")
        {
            if (i + 1 < argc)
            {
                options.output_path = std::string(argv[++i]);
            }
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
    auto options = resolve_compilation_options_from_args(argc, argv);
    options.mode = CompilationMode::COMPILE;

    Program program;

    program.parse_files(options.source_files);

    return program.compile(options);
}

// `cstride -r <...>` or `cstride --run <...>`
int stride::cli::resolve_run_command(const int argc, char** argv)
{
    auto options = resolve_compilation_options_from_args(argc, argv);
    options.mode = CompilationMode::COMPILE_JIT;

    Program program;

    program.parse_files(options.source_files);

    return program.compile_jit(options);
}
