#pragma once

#include "ast/parsing_context.h"
#include "ast/nodes/types.h"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

namespace stride::stl
{
    /// Standard library functions implemented using C++ STL and exposed to Stride programs.
    void register_internal_symbols(
        const std::shared_ptr<ast::ParsingContext>& context);

    /// Manually register internal runtime symbols into the JIT's main dynamic library.
    void register_runtime_symbols(llvm::orc::LLJIT* jit);
} // namespace stride::stl
