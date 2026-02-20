#pragma once
#include "ast/modifiers.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token_set.h"
#include "ast_node.h"

#include <utility>

namespace stride::ast
{
    class AstEnumerableMember : public IAstNode
    {
        friend class AstEnumerable;

        std::string _name;
        std::unique_ptr<AstLiteral> _value;

    public:
        explicit AstEnumerableMember(
            const SourceLocation& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string name,
            std::unique_ptr<AstLiteral> value) :
            IAstNode(source, context),
            _name(std::move(name)),
            _value(std::move(value)) {}

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        AstLiteral& value() const
        {
            return *this->_value;
        }

        std::string to_string() override;
    };

    class AstEnumerable : public IAstNode
    {
        const std::vector<std::unique_ptr<AstEnumerableMember>> _members;
        std::string _name;

    public:
        explicit AstEnumerable(
            const SourceLocation& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<AstEnumerableMember>> members,
            std::string name) :
            IAstNode(source, context),
            _members(std::move(members)),
            _name(std::move(name)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstEnumerableMember>>&
        get_members() const
        {
            return this->_members;
        }

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_name;
        }

        std::string to_string() override;
    };

    std::unique_ptr<AstEnumerableMember> parse_enumerable_member(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& tokens);

    std::unique_ptr<AstEnumerable> parse_enumerable_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        VisibilityModifier modifier);
} // namespace stride::ast
