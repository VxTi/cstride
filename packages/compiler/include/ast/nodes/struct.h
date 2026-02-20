#pragma once
#include <utility>

#include "ast_node.h"
#include "types.h"
#include "ast/modifiers.h"
#include "ast/parsing_context.h"

namespace stride::ast
{
    class AstStructMember : public IAstNode
    {
        Symbol _struct_member_symbol;
        std::unique_ptr<IAstType> _type;

    public:
        AstStructMember(
            const std::shared_ptr<ParsingContext>& context,
            Symbol struct_member_symbol,
            std::unique_ptr<IAstType> type
        ) :
            IAstNode(struct_member_symbol.symbol_position, context),
            _struct_member_symbol(std::move(struct_member_symbol)),
            _type(std::move(type)) {}

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return this->_struct_member_symbol.name; }

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
            const SourceLocation &source,
            const std::shared_ptr<ParsingContext>& context,
            std::string name,
            std::unique_ptr<IAstType> reference
        )
            :
            IAstNode(source, context),
            _name(std::move(name)),
            _reference(std::move(reference)) {}

        explicit AstStruct(
            const SourceLocation &source,
            const std::shared_ptr<ParsingContext>& context,
            std::string name,
            std::vector<std::unique_ptr<AstStructMember>> members
        ) :
            IAstNode(source, context),
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
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;
    };

    std::unique_ptr<AstStruct> parse_struct_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& tokens,
        VisibilityModifier modifier
    );
}
