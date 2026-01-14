#include "ast/nodes/literals.h"

using namespace stride::ast;

llvm::Value* StringLiteral::codegen()
{
    return nullptr;
}

std::unique_ptr<AstNode> StringLiteral::try_parse(const Scope& scope, TokenSet& tokens)
{
    const auto sym = tokens.expect(TokenType::STRING_LITERAL);

    const auto concatenated = sym.lexeme.substr(1, sym.lexeme.size() - 2);

    return std::make_unique<StringLiteral>(StringLiteral(concatenated));
}

std::string StringLiteral::to_string()
{
    return std::format("StringLiteral({})", value);
}
