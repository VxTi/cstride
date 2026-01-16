#pragma once

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

namespace stride::ast
{
    class ISynthesisable
    {
    public:
        virtual ~ISynthesisable() = default;
        virtual llvm::Value* codegen(
            llvm::Module* module,
            llvm::LLVMContext& context,
            llvm::IRBuilder<>* builder
        ) = 0;
    };

    class IAstNode
    {
    public:
        virtual ~IAstNode() = default;

        [[nodiscard]] virtual std::string to_string() = 0;
    };
}
