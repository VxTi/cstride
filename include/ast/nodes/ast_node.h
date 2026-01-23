#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "files.h"

namespace stride::ast
{
    // If we include the header, it'll cause circular references, and it'll break everything.
    class Scope;

    class ISynthesisable
    {
    public:
        virtual ~ISynthesisable() = default;

        virtual llvm::Value* codegen(
            const std::shared_ptr<Scope>& scope,
            llvm::Module* module,
            llvm::LLVMContext& context, llvm::IRBuilder<>* builder
        ) = 0;

        /// Utility function for defining symbols before they're referenced.
        virtual void resolve_forward_references(
            const std::shared_ptr<Scope>& scope,
            llvm::Module* module,
            llvm::LLVMContext& context, llvm::IRBuilder<>* builder
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
