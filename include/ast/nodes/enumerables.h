#pragma once
#include "ast_node.h"
#include "ast/nodes/literal_values.h"
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
        explicit AstEnumerableMember(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const Symbol& name, std::unique_ptr<AstLiteral> value
        )
            : IAstNode(source, source_offset),
              _name(name),
              _value(std::move(value)) {}

        [[nodiscard]]
        Symbol name() const { return this->_name; }

        [[nodiscard]]
        AstLiteral& value() const { return *this->_value; }

        static std::unique_ptr<AstEnumerableMember> try_parse_member(const Scope& scope, TokenSet& tokens);

        std::string to_string() override;
    };

    class AstEnumerable : public IAstNode
    {
        const std::vector<std::unique_ptr<AstEnumerableMember>> _members;
        const Symbol _name;

    public:
        explicit AstEnumerable(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::vector<std::unique_ptr<AstEnumerableMember>> members,
            Symbol name
        )
            :
            IAstNode(source, source_offset),
            _members(std::move(members)), _name(std::move(name)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstEnumerableMember>>& members() const
        {
            return this->_members;
        }

        [[nodiscard]]
        Symbol name() const { return this->_name; }

        std::string to_string() override;
    };

    std::unique_ptr<AstEnumerable> parse_enumerable_declaration(const Scope& scope, TokenSet& set);

    bool is_enumerable_declaration(const TokenSet& tokens);
}
