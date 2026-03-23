#pragma once
#include <memory>

namespace llvm::orc {
    class LLJIT;
}

namespace stride::ast {
    class SymbolTable;
}

namespace stride::runtime
{
    void register_runtime_symbols(const ast::SymbolTable* symbol_table);

    void register_jit_symbols(llvm::orc::LLJIT* jit);
}
