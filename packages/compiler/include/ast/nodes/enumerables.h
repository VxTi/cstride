#pragma once
#include <utility>

#include "ast_node.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstEnumerableMember : public IAstNode
    {
        friend class AstEnumerable;

        std::string _name;
        std::unique_ptr<AstLiteral> _value;

    public:
        explicit AstEnumerableMember(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::string name, std::unique_ptr<AstLiteral> value
        ) : IAstNode(source, source_position, scope),
            _name(std::move(name)),
            _value(std::move(value)) {}

        [[nodiscard]]
        const std::string& get_name() const { return this->_name; }

        [[nodiscard]]
        AstLiteral& value() const { return *this->_value; }

        std::string to_string() override;
    };

    class AstEnumerable : public IAstNode
    {
        const std::vector<std::unique_ptr<AstEnumerableMember>> _members;
        std::string _name;

    public:
        explicit AstEnumerable(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::vector<std::unique_ptr<AstEnumerableMember>> members,
            std::string name
        ) : IAstNode(source, source_position, scope),
            _members(std::move(members)), _name(std::move(name)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstEnumerableMember>>& get_members() const
        {
            return this->_members;
        }

        [[nodiscard]]
        const std::string& get_name() const { return this->_name; }

        std::string to_string() override;
    };

    std::unique_ptr<AstEnumerableMember> parse_enumerable_member(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& tokens
    );

    std::unique_ptr<AstEnumerable> parse_enumerable_declaration(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set
    );

    bool is_enumerable_declaration(const TokenSet& tokens);
}
