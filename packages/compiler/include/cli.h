#pragma once

namespace stride::cli
{
    int resolve_cli_command(int argc, char** argv);

    // `cstride -c <...>` or `cstride --compile <...>`
    int resolve_compile_command(int argc, char** argv);

    // `cstride install` or `cstride i`
    int resolve_install_command(int argc, char** argv);

    /// `cstride --typecheck` or `cstride -tc`
    int resolve_typecheck_command(int argc, char** argv);
}
