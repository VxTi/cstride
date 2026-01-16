#pragma once
#include "ast_node.h"
#include "ast/nodes/literals.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstEnumerableMember : IAstNode
    {
        friend class AstEnumerable;

        Symbol _name;
        std::unique_ptr<AstLiteral> _value;

    public:
        explicit AstEnumerableMember(const Symbol& name, std::unique_ptr<AstLiteral> value)
            : _name(name),
              _value(std::move(value))
        {
        }

        [[nodiscard]] Symbol name() const { return this->_name; }

        [[nodiscard]] AstLiteral& value() const { return *this->_value; }

        static std::unique_ptr<AstEnumerableMember> try_parse_member(const Scope& scope, TokenSet& tokens);

        std::string to_string() override;
    };

    class AstEnumerable : public IAstNode
    {
        const std::vector<std::unique_ptr<AstEnumerableMember>> _members;
        const Symbol _name;

    public:
        explicit AstEnumerable(std::vector<std::unique_ptr<AstEnumerableMember>> members, Symbol name) :
            _members(std::move(members)), _name(std::move(name))
        {
        }

        [[nodiscard]] const std::vector<std::unique_ptr<AstEnumerableMember>>& members() const
        {
            return this->_members;
        }

        [[nodiscard]] Symbol name() const { return this->_name; }

        static std::unique_ptr<AstEnumerable> try_parse(const Scope& scope, TokenSet& tokens);

        static bool can_parse(const TokenSet& tokens);

        std::string to_string() override;
    };
}
