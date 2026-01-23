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
std::unique_ptr<AstEnumerableMember> AstEnumerableMember::try_parse_member(std::shared_ptr<Scope> scope, TokenSet& tokens)
{
    const auto member_name_tok = tokens.expect(TokenType::IDENTIFIER);
    auto member_sym = member_name_tok.lexeme;

    scope->define_symbol(member_sym, IdentifiableSymbolType::ENUM_MEMBER);

    tokens.expect(TokenType::COLON, "Expected a colon after enum member name");

    auto value = parse_literal_optional(scope, tokens);

    if (!value.has_value())
    {
        tokens.throw_error("Expected a literal value for enum member");
    }

    tokens.expect(TokenType::COMMA, "Expected a comma after enum member value");

    return std::make_unique<AstEnumerableMember>(
        tokens.source(),
        member_name_tok.offset,
        scope,
        std::move(member_sym),
        std::move(value.value())
    );
}

std::string AstEnumerableMember::to_string()
{
    auto member_value_str = this->value().to_string();
    return std::format("{}: {}", this->get_name(), member_value_str);
}

std::string AstEnumerable::to_string()
{
    std::ostringstream imploded;

    if (this->get_members().empty())
    {
        return std::format("Enumerable {} (empty)", this->get_name());
    }

    imploded << this->get_members()[0]->to_string();
    for (size_t i = 1; i < this->get_members().size(); ++i)
    {
        imploded << "\n  " << this->get_members()[i]->to_string();
    }

    return std::format("Enumerable {} (\n  {}\n)", this->get_name(), imploded.str());
}


std::unique_ptr<AstEnumerable> stride::ast::parse_enumerable_declaration(std::shared_ptr<Scope> scope, TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_ENUM);
    const auto enumerable_name_tok = set.expect(TokenType::IDENTIFIER);
    auto enumerable_name = enumerable_name_tok.lexeme;

    scope->define_symbol(enumerable_name, IdentifiableSymbolType::ENUM);

    const auto opt_enum_body_subset = collect_block(set);

    if (!opt_enum_body_subset.has_value())
    {
        set.throw_error("Expected a block in enum declaration");
    }

    std::vector<std::unique_ptr<AstEnumerableMember>> members = {};

    auto nested_scope = std::make_shared<Scope>(scope, ScopeType::BLOCK);
    auto enum_body_subset = opt_enum_body_subset.value();

    while (enum_body_subset.has_next())
    {
        auto member = AstEnumerableMember::try_parse_member(nested_scope, enum_body_subset);
        members.push_back(std::move(member));
    }

    return std::make_unique<AstEnumerable>(
        set.source(),
        reference_token.offset,
        scope,
        std::move(members),
        enumerable_name
    );
}

bool stride::ast::is_enumerable_declaration(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_ENUM);
}
