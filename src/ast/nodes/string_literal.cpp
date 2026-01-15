#include "ast/nodes/literals.h"

using namespace stride::ast;

llvm::Value* StringLiteral::codegen()
{
    return nullptr;
}

std::optional<std::unique_ptr<AstNode>> StringLiteral::try_parse(Scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::STRING_LITERAL))
    {
        const auto sym = tokens.next();

        const auto concatenated = sym.lexeme.substr(1, sym.lexeme.size() - 2);

        return std::make_unique<StringLiteral>(StringLiteral(concatenated));
    }
    return std::nullopt;
}

std::string StringLiteral::to_string()
{
    return std::format("StringLiteral({})", value);
}
