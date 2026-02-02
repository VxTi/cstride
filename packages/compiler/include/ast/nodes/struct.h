#pragma once
#include <utility>

#include "ast_node.h"
#include "types.h"
#include "ast/symbol_registry.h"

namespace stride::ast
{
    class AstStructMember : public IAstNode
    {
        std::string _name;
        std::unique_ptr<IAstType> _type;

    public:
        AstStructMember(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::string name,
            std::unique_ptr<IAstType> type
        ) :
            IAstNode(source, source_position, registry),
            _name(std::move(name)),
            _type(std::move(type)) {}

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return this->_name; }

        [[nodiscard]]
        IAstType& get_type() const { return *this->_type; }

        void validate() override;
    };

    class AstStruct
        : public IAstNode,
          public ISynthesisable
    {
        std::string _name;
        std::vector<std::unique_ptr<AstStructMember>> _members;

        // Whether this struct references another one
        // This can be used for declaring a type with the data layout
        // of another
        std::optional<std::unique_ptr<IAstType>> _reference;

    public:
        explicit AstStruct(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::string name,
            std::unique_ptr<IAstType> reference
        )
            :
            IAstNode(source, source_position, registry),
            _name(std::move(name)),
            _reference(std::move(reference)) {}

        explicit AstStruct(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::string name,
            std::vector<std::unique_ptr<AstStructMember>> members
        ) :
            IAstNode(source, source_position, registry),
            _name(std::move(name)),
            _members(std::move(members)),
            _reference(std::nullopt) {}

        [[nodiscard]]
        bool is_reference_type() const { return _reference.has_value(); }

        [[nodiscard]]
        IAstType* get_reference_type() const { return _reference->get(); }

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return _name; }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstStructMember>>& get_members() const { return this->_members; }

        void validate() override;

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;
    };

    std::unique_ptr<AstStruct> parse_struct_declaration(const std::shared_ptr<SymbolRegistry>& registry, TokenSet& tokens);

    bool is_struct_declaration(const TokenSet& tokens);
}
