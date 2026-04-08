#pragma once

#include "files.h"

#include <optional>

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
    class SymbolTable;

    template <class TBase>
    class Cloneable
    {
    public:
        virtual std::unique_ptr<TBase> clone()
        {
            throw std::runtime_error("clone() not implemented for this type");
        }

        template <typename T>
        std::unique_ptr<T> clone_as()
        {
            static_assert(std::is_base_of_v<TBase, T>,
                          "T must be a subclass of IAstNode");

            auto base = this->clone();
            if (const auto ptr = dynamic_cast<T*>(base.get()))
            {
                base.release();
                return std::unique_ptr<T>(ptr);
            }

            throw std::bad_cast{};
        }

        virtual ~Cloneable() = default;
    };

    class IAstNode
        : public Cloneable<IAstNode>
    {
        const SourcePosition _source_position;

    public:
        explicit IAstNode(const SourcePosition& source) :
            _source_position(source) {}

        IAstNode(const IAstNode& other) = default;

        ~IAstNode() override = default;

        virtual std::string to_string() = 0;

        virtual void validate(SymbolTable* symbol_table) {}

        [[nodiscard]]
        std::shared_ptr<SourceFile> get_source() const
        {
            return this->_source_position.source;
        }

        [[nodiscard]]
        SourcePosition get_source_position() const
        {
            return this->_source_position;
        }

        virtual llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) = 0;

        /// Utility function for defining symbols before they're referenced.
        virtual void resolve_forward_references(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) {}

        std::unique_ptr<IAstNode> clone() override;
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
        virtual std::optional<std::unique_ptr<IAstNode>> reduce() = 0;

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
