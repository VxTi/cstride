#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace stride::project
{
    struct Dependency
    {
        std::string name;
        std::string version;
        std::string path;
    };

    enum class Mode
    {
        COMPILE_NATIVE,
        COMPILE_JIT
    };

    struct Config
    {
        /**
         * The name of the generated binary / application
         */
        std::string name;

        /**
         * The version of the generated binary / application
         */
        std::string version;

        /**
         * Path of the main file. Will default to <code>./src/main.sr</code>
         */
        std::string main;
        /**
         * The path to the main source file for the project
         * Defaults to <code>./build/</code>
         */
        std::string buildPath;
        /**
         * A list of dependencies to include in the project
         */
        std::vector<Dependency> dependencies;
        /**
         * The target platform for the project.
         * This can be used to determine which libraries to link against, which compiler flags to
         * use, etc. Will default to the current platform
         */
        std::string target;

        /**
         * The mode of the compiler. This can be either <code>COMPILE_NATIVE</code> or
         * <code>COMPILE_JIT</code>.
         */
        Mode mode;
    };

    std::optional<Config> read_config(const std::string& path);
} // namespace stride::project
