#include "ast/nodes/function_invocation.h"
using namespace stride::ast;


std::string AstFunctionInvocation::to_string()
{
    return "FunctionInvocation: " + function_name.to_string();
}

llvm::Value* AstFunctionInvocation::codegen()
{
    return nullptr;
}

bool AstFunctionInvocation::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::IDENTIFIER);
}

Symbol consume_function_name(TokenSet& tokens)
{
    const auto initial = tokens.expect(TokenType::IDENTIFIER, "Expected function name").lexeme;
    std::vector function_segments = {initial};

    while (tokens.peak_next_eq(TokenType::DOUBLE_COLON))
    {
        tokens.next();
        const auto next = tokens.expect(TokenType::IDENTIFIER, "Expected function name segment").lexeme;
        function_segments.push_back(next);
    }

    return Symbol::from_segments(function_segments);
}

std::unique_ptr<AstFunctionInvocation> AstFunctionInvocation::try_parse(const Scope& scope, TokenSet& tokens)
{
    const auto function_name = consume_function_name(tokens);
    tokens.expect(TokenType::LPAREN, "Expected '(' after function name for invocation");

    tokens.expect(TokenType::RPAREN, "Expected ')' after function invocation");

    return std::make_unique<AstFunctionInvocation>(function_name);
}
