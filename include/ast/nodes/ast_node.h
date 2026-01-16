#pragma once

#include <llvm/IR/Value.h>

namespace stride::ast
{
    class ISynthesisable
    {
    public:
        virtual ~ISynthesisable() = default;
        virtual llvm::Value* codegen() = 0;
    };

    class IAstNode
    {
    public:
        virtual ~IAstNode() = default;

        [[nodiscard]] virtual std::string to_string() = 0;
    };
}
