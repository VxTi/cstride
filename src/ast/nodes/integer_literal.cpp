#include "ast/nodes/literals.h"

using namespace stride::ast;

std::unique_ptr<AstNode> IntegerLiteral::try_parse(const Scope& scope, TokenSet& tokens)
{
    const auto sym = tokens.expect(TokenType::INTEGER_LITERAL);
    return std::make_unique<IntegerLiteral>(std::stoi(sym.lexeme));
}

std::string IntegerLiteral::to_string()
{
    return std::format("IntegerLiteral({})", value);
}

llvm::Value* IntegerLiteral::codegen()
{
    return nullptr;
}
