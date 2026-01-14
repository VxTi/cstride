#pragma once

#include <llvm/IR/Value.h>

namespace stride::ast
{
    class AstNode
    {
    public:
        virtual ~AstNode() = default;

        [[nodiscard]] virtual std::string to_string() = 0;

        virtual llvm::Value* codegen() = 0;
    };
}
