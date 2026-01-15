#include "ast/nodes/literals.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstNode>> IntegerLiteral::try_parse(Scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::INTEGER_LITERAL))
    {
        const auto next = tokens.next();
        return std::make_unique<IntegerLiteral>(std::stoi(next.lexeme));
    }
    return std::nullopt;
}

std::string IntegerLiteral::to_string()
{
    return std::format("IntegerLiteral({})", value);
}

llvm::Value* IntegerLiteral::codegen()
{
    return nullptr;
}
