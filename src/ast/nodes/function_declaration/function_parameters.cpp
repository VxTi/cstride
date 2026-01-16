#include "ast/nodes/function_signature.h"

using namespace stride::ast;

std::string AstFunctionParameterNode::to_string()
{
    return name.value;
}

std::unique_ptr<AstFunctionParameterNode> AstFunctionParameterNode::try_parse(
    [[maybe_unused]] const Scope& scope,
    TokenSet& tokens
)
{
    const auto param_name = Symbol(tokens.expect(TokenType::IDENTIFIER).lexeme);

    tokens.expect(TokenType::COLON);

    std::unique_ptr<AstType> type_ptr = try_parse_type(tokens);

    return std::move(std::make_unique<AstFunctionParameterNode>(param_name, std::move(type_ptr)));
}
