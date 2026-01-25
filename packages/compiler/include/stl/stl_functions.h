#pragma once

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include "ast/scope.h"
#include "ast/nodes/types.h"

namespace stride::stl
{
    /// Standard library functions implemented using C++ STL and exposed to Stride programs.

    void llvm_jit_define_functions(llvm::orc::LLJIT* jit);

    void predefine_symbols(const std::shared_ptr<ast::Scope>& global_scope);

    void llvm_insert_function_definitions(llvm::Module* module, llvm::LLVMContext& context);
}
