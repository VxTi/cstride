#include <sstream>

#include "ast/nodes/struct.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

std::unique_ptr<AstStructMember> try_parse_struct_member(
    const Scope& scope,
    TokenSet& tokens
)
{
    const auto struct_member_name = tokens.expect(TokenType::IDENTIFIER, "Expected struct member name");
    const auto struct_member_sym = Symbol(struct_member_name.lexeme);

    scope.try_define_scoped_symbol(*tokens.source(), struct_member_name, struct_member_sym);

    tokens.expect(TokenType::COLON);

    auto struct_member_type = types::try_parse_type(tokens);
    tokens.expect(TokenType::SEMICOLON);

    return std::make_unique<AstStructMember>(struct_member_sym, std::move(struct_member_type));
}

bool AstStruct::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_STRUCT);
}

std::unique_ptr<AstStruct> AstStruct::try_parse(const Scope& scope, TokenSet& tokens)
{
    tokens.expect(TokenType::KEYWORD_STRUCT);
    const auto struct_name = tokens.expect(TokenType::IDENTIFIER, "Expected struct name");
    const auto struct_sym = Symbol(struct_name.lexeme);

    scope.try_define_global_symbol(*tokens.source(), struct_name, struct_sym);

    // Might be a reference to another type
    // This will parse a definition like:
    //
    // struct Foo = Bar;
    if (tokens.peak_next_eq(TokenType::EQUALS))
    {
        tokens.next();
        auto reference_sym = types::try_parse_type(tokens);

        tokens.expect(TokenType::SEMICOLON);

        return std::make_unique<AstStruct>(struct_sym, std::move(reference_sym));
    }

    auto struct_body_set = AstBlock::collect_block(tokens);
    std::vector<std::unique_ptr<AstStructMember>> members = {};

    if (struct_body_set.has_value())
    {
        const auto nested_scope = Scope(scope, ScopeType::BLOCK);
        while (struct_body_set.value().has_next())
        {
            auto member = try_parse_struct_member(nested_scope, struct_body_set.value());
            members.push_back(std::move(member));
        }
    }

    return std::make_unique<AstStruct>(struct_sym, std::move(members));
}


std::string AstStructMember::to_string()
{
    return std::format("{}: {}", this->name().to_string(), this->type().to_string());
}

std::string AstStruct::to_string()
{
    if (this->is_reference())
    {
        return std::format("Struct({}) (reference to {})", this->name().to_string(), this->reference()->to_string());
    }
    if (this->members().empty())
    {
        return std::format("Struct({}) (empty)", this->name().to_string());
    }

    std::ostringstream imploded;

    imploded << this->members()[0]->to_string();
    for (size_t i = 1; i < this->members().size(); ++i)
    {
        imploded << "\n  " << this->members()[i]->to_string();
    }
    return std::format("Struct({}) (\n  {}\n)", this->name().to_string(), imploded.str());
}
