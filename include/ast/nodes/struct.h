#pragma once
#include "types.h"
#include "ast_node.h"
#include "ast/scope.h"
#include "ast/identifiers.h"

namespace stride::ast
{
    class AstStructMember : public IAstNode
    {
        std::string _name;
        std::unique_ptr<types::AstType> _type;

    public:
        AstStructMember(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::string& name,
            std::unique_ptr<types::AstType> type
        ) :
            IAstNode(source, source_offset),
            _name(name),
            _type(std::move(type)) {}

        std::string to_string() override;

        std::string get_name() const { return this->_name; }

        types::AstType& get_type() const { return *this->_type; }
    };

    class AstStruct : public IAstNode
    {
        std::string _name;
        std::vector<std::unique_ptr<AstStructMember>> _cases;
        std::optional<std::unique_ptr<AstStructMember>> _default_case;

        // Whether this struct references another one
        // This can be used for declaring a type with the data layout
        // of another
        std::optional<std::unique_ptr<types::AstType>> _reference;

    public:
        AstStruct(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::string& name,
            std::unique_ptr<types::AstType> reference
        )
            :
            IAstNode(source, source_offset),
            _name(name),
            _reference(std::move(reference)) {}

        AstStruct(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::string& name,
            std::vector<std::unique_ptr<AstStructMember>> members,
            std::optional<std::unique_ptr<AstStructMember>> default_case = std::nullopt
        ) :
            IAstNode(source, source_offset),
            _name(name),
            _cases(std::move(members)),
            _default_case(std::move(default_case)),
            _reference(std::nullopt) {}

        bool is_reference_type() const { return _reference.has_value(); }

        types::AstType* get_reference_type() const { return _reference->get(); }

        std::string to_string() override;

        std::string get_name() const { return _name; }

        const std::vector<std::unique_ptr<AstStructMember>>& get_members() const { return this->_cases; }
    };

    std::unique_ptr<AstStruct> parse_struct_declaration(const Scope& scope, TokenSet& tokens);

    bool is_struct_declaration(const TokenSet& tokens);
}
