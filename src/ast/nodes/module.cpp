#include "ast/nodes/module.h"

using namespace stride::ast;

llvm::Value* AstModule::codegen()
{
    return nullptr;
}

std::string AstModule::to_string()
{
    return std::format("Module ({})", this->name().to_string());
}

bool AstModule::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_MODULE);
}

std::unique_ptr<AstModule> AstModule::try_parse(
    [[maybe_unused]] const Scope& scope,
    TokenSet& tokens
)
{
    if (
        const auto mod_token = tokens.expect(TokenType::KEYWORD_MODULE);
        mod_token.offset != 0
    )
    {
        tokens.except(
            mod_token,
            ErrorType::SYNTAX_ERROR,
            "Module declaration must be at the beginning of the file"
        );
    }

    const auto module_name_token = tokens.expect(
        TokenType::IDENTIFIER,
        "Expected module name after 'mod' keyword"
    );
    tokens.expect(TokenType::SEMICOLON);
    const auto module_name = Symbol(module_name_token.lexeme);

    return std::make_unique<AstModule>(module_name);
}
