#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include <utility>

#include "files.h"

namespace stride::ast
{
#define MAX_RECURSION_DEPTH 100

    class AstBlock;
    class ParsingContext;

    class ISynthesisable
    {
    public:
        virtual ~ISynthesisable() = default;

        virtual llvm::Value* codegen(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) = 0;

        /// Utility function for defining symbols before they're referenced.
        virtual void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) {}
    };

    class IAstNode
    {
        const SourceLocation _source_position;
        const std::shared_ptr<ParsingContext> _scope;

    public:
        explicit IAstNode(
            SourceLocation source,
            const std::shared_ptr<ParsingContext>& context
        )
            : _source_position(std::move(source)),
              _scope(context) {}

        virtual ~IAstNode() = default;

        virtual std::string to_string() = 0;

        virtual void validate() {}

        [[nodiscard]]
        std::shared_ptr<SourceFile> get_source() const { return this->_source_position.source; }

        [[nodiscard]]
        std::shared_ptr<ParsingContext> get_context() const { return this->_scope; }

        [[nodiscard]]
        SourceLocation get_source_position() const { return this->_source_position; }
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

    class IAstContainer
    {
    public:
        virtual ~IAstContainer() = default;

        [[nodiscard]]
        virtual AstBlock* get_body() = 0;
    };
}
