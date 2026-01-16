#include "ast/nodes/enumerables.h"

#include <sstream>
#include <format>

#include "ast/nodes/blocks.h"

using namespace stride::ast;

/**
 * Parses member entries with the following sequence:
 * <code>
 * IDENTIFIER: <literal value>;
 * </code>
 */
std::unique_ptr<AstEnumerableMember> AstEnumerableMember::try_parse_member(const Scope& scope, TokenSet& tokens)
{
    const auto member_name_tok = tokens.expect(TokenType::IDENTIFIER);
    auto member_sym = Symbol(member_name_tok.lexeme);

    scope.try_define_scoped_symbol(*tokens.source().get(), member_name_tok, member_sym);

    tokens.expect(TokenType::COLON, "Expected a colon after enum member name");

    auto value = AstLiteral::try_parse(scope, tokens);

    if (!value.has_value())
    {
        tokens.except("Expected a literal value for enum member");
    }

    tokens.expect(TokenType::COMMA, "Expected a comma after enum member value");

    return std::make_unique<AstEnumerableMember>(
        std::move(member_sym),
        std::move(value.value())
    );
}

std::string AstEnumerableMember::to_string()
{
    auto member_value_str = this->value().to_string();
    return std::format("{}: {}", this->name().to_string(), member_value_str);
}

std::string AstEnumerable::to_string()
{
    std::ostringstream imploded;

    if (this->members().empty())
    {
        return std::format("Enumerable {} (empty)", this->name().to_string());
    }

    imploded << this->members()[0]->to_string();
    for (size_t i = 1; i < this->members().size(); ++i)
    {
        imploded << "\n  " << this->members()[i]->to_string();
    }

    return std::format("Enumerable {} (\n  {}\n)", this->name().to_string(), imploded.str());
}


std::unique_ptr<AstEnumerable> AstEnumerable::try_parse(const Scope& scope, TokenSet& tokens)
{
    tokens.expect(TokenType::KEYWORD_ENUM);
    const auto enumerable_name = tokens.expect(TokenType::IDENTIFIER);
    const auto enumerable_sym = Symbol(enumerable_name.lexeme);

    scope.try_define_global_symbol(*tokens.source(), enumerable_name, enumerable_sym);

    const auto opt_enum_body_subset = AstBlockNode::collect_block(tokens);

    if (!opt_enum_body_subset.has_value())
    {
        tokens.except("Expected a block in enum declaration");
    }

    std::vector<std::unique_ptr<AstEnumerableMember>> members = {};

    auto nested_scope = Scope(scope, ScopeType::BLOCK);
    auto enum_body_subset = opt_enum_body_subset.value();

    while (enum_body_subset.has_next())
    {
        auto member = AstEnumerableMember::try_parse_member(nested_scope, enum_body_subset);
        members.push_back(std::move(member));
    }

    return std::make_unique<AstEnumerable>(
        std::move(members),
        Symbol(enumerable_name.lexeme)
    );
}

bool AstEnumerable::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_ENUM);
}
