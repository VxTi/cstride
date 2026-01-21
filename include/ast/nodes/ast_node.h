#pragma once

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>

#include "files.h"

namespace stride::ast
{
    template <typename... T>
    using std::unique_ptr = std::unique_ptr<T...>;

    template <typename... T>
    using std::shared_ptr = std::shared_ptr<T...>;

    template <typename... T>
    using option = std::optional<T...>;

    class ISynthesisable
    {
    public:
        virtual ~ISynthesisable() = default;

        virtual llvm::Value* codegen(
            llvm::Module* module,
            llvm::LLVMContext& context,
            llvm::IRBuilder<>* builder
        ) = 0;

        virtual void define_symbols(
            llvm::Module* module,
            llvm::LLVMContext& context,
            llvm::IRBuilder<>* builder
        ) {}
    };

    class IAstNode
    {
    public:
        explicit IAstNode(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset
        )
            : source(source),
              source_offset(source_offset) {}

        virtual ~IAstNode() = default;

        virtual std::string to_string() = 0;

        virtual void validate() {}

        const std::shared_ptr<SourceFile> source;

        const int source_offset;
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
