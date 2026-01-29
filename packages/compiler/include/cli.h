#pragma once

#include <string>
#include <vector>

namespace stride::cli
{
    /**
     * @brief Enumerates the available modes for the compilation process.
     *
     * The CompilationMode enumeration defines the possible operating modes
     * for the compilation process. These modes determine how the code will
     * be processed:
     * - <code>INTERPRET</code>: Executes the code directly without producing an output binary.
     * - <code>COMPILE</code>: Translates the code into an executable binary.
     */
    enum class CompilationMode
    {
        INTERPRET,
        COMPILE_JIT
    };

    typedef struct CompilationOptions
    {
        /**
         * @brief Stores the list of include paths for the compilation process.
         *
         * The include_paths variable contains a collection of directory paths
         * that are searched during the compilation process to locate header files
         * and other required resources. Each string in the vector represents a single
         * directory path to be added to the include search paths.
         */
        std::vector<std::string> include_paths;

        /**
         * @brief Represents the mode of compilation to be used.
         *
         * The CompilationMode variable determines whether the process should
         * operate in interpretive mode or compilation mode
         * It can take one of the values defined in the <code>stride::ast::CompilationMode</code>
         * enumeration, such as <code>CompilationMode::INTERPRET</code> or <code>CompilationMode::COMPILE</code>.
         */
        CompilationMode mode;

        /**
         * @brief Indicates whether debug mode is enabled for the compilation process.
         *
         * When debug_mode is set to true, additional debugging information and
         * intermediate steps are included in the compilation output, which can
         * be useful for troubleshooting and understanding the compilation process.
         */
        bool debug_mode;
    } CompilationOptions;

    /**
     * Will attempt to resolve compilation options from command-line arguments
     * into a <code>stride::ast::CompilationOptions</code> structure.
     */
    CompilationOptions resolve_compilation_options_from_args(int argc, char** argv);

    int resolve_cli_command(int argc, char** argv);

    // `cstride -c <...>` or `cstride --compile <...>`
    int resolve_compile_command(int argc, char** argv);

    // `cstride install` or `cstride i`
    int resolve_install_command(int argc, char** argv);

    /// `cstride --typecheck` or `cstride -tc`
    int resolve_typecheck_command(int argc, char** argv);
}
