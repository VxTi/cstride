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
    void register_runtime_symbols(const std::shared_ptr<ast::SymbolTable>& context);

    void register_jit_symbols(llvm::orc::LLJIT* jit);
}
