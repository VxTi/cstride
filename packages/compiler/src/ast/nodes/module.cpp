#include "ast/nodes/module.h"

#include "ast/identifiers.h"

using namespace stride::ast;

std::string AstModule::to_string()
{
    return std::format("Module ({})", this->get_name());
}

bool stride::ast::is_module_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_MOD);
}

std::unique_ptr<AstModule> stride::ast::parse_module_statement(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& tokens
)
{
    const auto reference_token = tokens.expect(TokenType::KEYWORD_MOD);

    if (reference_token.offset != 0)
    {
        tokens.throw_error(
            reference_token,
            ErrorType::SYNTAX_ERROR,
            "Module declaration must be at the beginning of the file"
        );
    }


    const auto module_name_token = tokens.expect(
        TokenType::IDENTIFIER,
        "Expected module name after 'mod' keyword"
    );
    std::vector<std::string> segments = {module_name_token.lexeme};

    while (tokens.peak_next_eq(TokenType::DOUBLE_COLON))
    {
        tokens.next();
        const auto next = tokens.expect(TokenType::IDENTIFIER, "Expected module name segment");

        segments.push_back(next.lexeme);
    }

    tokens.expect(TokenType::SEMICOLON);
    const auto module_name = internal_identifier_from_segments(segments);

    return std::make_unique<AstModule>(
        tokens.source(),
        reference_token.offset,
        scope,
        module_name
    );
}
