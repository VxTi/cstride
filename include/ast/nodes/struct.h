#pragma once
#include "primitive_type.h"
#include "ast_node.h"
#include "ast/scope.h"
#include "ast/symbol.h"

namespace stride::ast
{
    class AstStructMember : public IAstNode
    {
        Symbol _name;
        std::unique_ptr<types::AstType> _type;

    public:
        AstStructMember(const Symbol& name, std::unique_ptr<types::AstType> type) :
            _name(name),
            _type(std::move(type))
        {
        }

        std::string to_string() override;

        Symbol name() const { return this->_name; }
        types::AstType& type() const { return *this->_type; }
    };

    class AstStruct : public IAstNode
    {
        Symbol _name;
        std::vector<std::unique_ptr<AstStructMember>> _cases;
        std::optional<std::unique_ptr<AstStructMember>> _default_case;

        // Whether this struct references another one
        // This can be used for declaring a type with the data layout
        // of another
        std::optional<std::unique_ptr<types::AstType>> _reference;

    public:
        AstStruct(
            const Symbol& name,
            std::unique_ptr<types::AstType> reference
        ) : _name(name), _reference(std::move(reference))
        {
        }

        AstStruct(
            const Symbol& name,
            std::vector<std::unique_ptr<AstStructMember>> members,
            std::optional<std::unique_ptr<AstStructMember>> default_case = std::nullopt
        ) : _name(name),
            _cases(std::move(members)),
            _default_case(std::move(default_case)),
            _reference(std::nullopt)
        {
        }

        bool is_reference() const { return _reference.has_value(); }

        types::AstType* reference() const { return _reference->get(); }

        std::string to_string() override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstStruct> try_parse(const Scope& scope, TokenSet& tokens);

        Symbol name() const { return _name; }

        const std::vector<std::unique_ptr<AstStructMember>>& members() const { return this->_cases; }
    };
}
