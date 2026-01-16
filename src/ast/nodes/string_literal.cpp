#include "ast/nodes/literals.h"

using namespace stride::ast;

llvm::Value* AstStringLiteral::codegen()
{
    return nullptr;
}

std::optional<std::unique_ptr<AstLiteral>> AstStringLiteral::try_parse_optional(
    [[maybe_unused]] const Scope& scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::STRING_LITERAL))
    {
        const auto sym = tokens.next();

        const auto concatenated = sym.lexeme.substr(1, sym.lexeme.size() - 2);

        return std::make_unique<AstStringLiteral>(AstStringLiteral(concatenated));
    }
    return std::nullopt;
}

std::string AstStringLiteral::to_string()
{
    return std::format("StringLiteral({})", value);
}
