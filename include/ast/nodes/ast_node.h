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

        [[nodiscard]]
        virtual std::string to_string() = 0;
    };

    class IReducible
    {
    public:
        virtual ~IReducible() = default;

        /**
         * Reduces the current node to a simpler form.
         * This is part of the reduction process, where complex nodes are simplified
         * to make further analysis or code generation easier.
         * @return The reduced node.
         */
        virtual IAstNode* reduce() = 0;

        /**
         * Checks if the node can be reduced.
         * @return True if the node can be reduced, false otherwise.
         */
        virtual bool is_reducible() = 0;
    };
}
