#pragma once

#include "files.h"

namespace llvm
{
    class Module;
    class Value;
    class IRBuilderBase;
}

#define MAX_RECURSION_DEPTH 100

namespace stride::ast
{

    class AstBlock;
    class ParsingContext;

    class IAstNode
    {
        const SourceFragment _source_position;
        const std::shared_ptr<ParsingContext> _context;

    public:
        explicit IAstNode(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context
        ) :
            _source_position(source),
            _context(context) {}

        virtual ~IAstNode() = default;

        virtual std::string to_string() = 0;

        virtual void validate() {}

        [[nodiscard]]
        std::shared_ptr<SourceFile> get_source() const
        {
            return this->_source_position.source;
        }

        [[nodiscard]]
        std::shared_ptr<ParsingContext> get_context() const
        {
            return this->_context;
        }

        [[nodiscard]]
        SourceFragment get_source_fragment() const
        {
            return this->_source_position;
        }

        virtual llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) = 0;

        /// Utility function for defining symbols before they're referenced.
        virtual void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) {}

        virtual std::unique_ptr<IAstNode> clone() = 0;

        template <typename T>
        std::unique_ptr<T> clone_as()
        {
            static_assert(std::is_base_of_v<IAstNode, T>,
                          "T must be a subclass of IAstNode");

            auto base = this->clone();
            if (const auto ptr = dynamic_cast<T*>(base.get()))
            {
                base.release();
                return std::unique_ptr<T>(ptr);
            }

            throw std::bad_cast{};
        }
    };

    class IAstStatement
    {
        friend class AstBlock;

    public:
        virtual ~IAstStatement() = default;
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
} // namespace stride::ast
