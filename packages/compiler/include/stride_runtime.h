#pragma once
#include <memory>

namespace stride::ast {
    class ParsingContext;
}

namespace stride::runtime
{
    void register_runtime_symbols(const std::shared_ptr<ast::ParsingContext>& context);
}
