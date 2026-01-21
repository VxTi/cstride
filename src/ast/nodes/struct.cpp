#include <sstream>

#include "ast/nodes/struct.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

std::unique_ptr<AstStructMember> try_parse_struct_member(
    const Scope& scope,
    TokenSet& set
)
{
    const auto struct_member_name_tok = set.expect(TokenType::IDENTIFIER, "Expected struct member name");
    const auto struct_member_name = struct_member_name_tok.lexeme;

    scope.try_define_scoped_symbol(*set.source(), struct_member_name_tok, struct_member_name, SymbolType::STRUCT);

    set.expect(TokenType::COLON);

    auto struct_member_type = parse_primitive_type(set);
    set.expect(TokenType::SEMICOLON);

    return std::make_unique<AstStructMember>(
        set.source(),
        struct_member_name_tok.offset,
        struct_member_name,
        std::move(struct_member_type)
    );
}

bool stride::ast::is_struct_declaration(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_STRUCT);
}

std::unique_ptr<AstStruct> stride::ast::parse_struct_declaration(const Scope& scope, TokenSet& tokens)
{
    const auto reference_token = tokens.expect(TokenType::KEYWORD_STRUCT);
    const auto struct_name_tok = tokens.expect(TokenType::IDENTIFIER, "Expected struct name");
    const auto struct_name = struct_name_tok.lexeme;

    scope.try_define_global_symbol(*tokens.source(), struct_name_tok, struct_name, SymbolType::STRUCT);

    // Might be a reference to another type
    // This will parse a definition like:
    //
    // struct Foo = Bar;
    if (tokens.peak_next_eq(TokenType::EQUALS))
    {
        tokens.next();
        auto reference_sym = parse_primitive_type(tokens);

        tokens.expect(TokenType::SEMICOLON);

        return std::make_unique<AstStruct>(
            tokens.source(),
            reference_token.offset,
            struct_name,
            std::move(reference_sym)
        );
    }

    auto struct_body_set = collect_block(tokens);
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

    return std::make_unique<AstStruct>(
        tokens.source(), reference_token.offset,
        struct_name,
        std::move(members)
    );
}


std::string AstStructMember::to_string()
{
    return std::format("{}: {}", this->get_name(), this->get_type().to_string());
}

std::string AstStruct::to_string()
{
    if (this->is_reference_type())
    {
        return std::format("Struct({}) (reference to {})", this->get_name(), this->get_reference_type()->to_string());
    }
    if (this->get_members().empty())
    {
        return std::format("Struct({}) (empty)", this->get_name());
    }

    std::ostringstream imploded;

    imploded << this->get_members()[0]->to_string();
    for (size_t i = 1; i < this->get_members().size(); ++i)
    {
        imploded << "\n  " << this->get_members()[i]->to_string();
    }
    return std::format("Struct({}) (\n  {}\n)", this->get_name(), imploded.str());
}
