#include "ast/nodes/function_definition.h"

#include "ast/nodes/root_node.h"

using namespace stride::ast;

std::string AstFunctionDefinitionNode::to_string()
{
    std::string params;
    for (const auto& param : parameters_)
    {
        if (!params.empty())
            params += ", ";
        params += param->to_string();
    }
    return "FunctionDefinition(name: " + this->name_.value + ", parameters: [" + params + "], body: NOT_IMPLEMENTED" +
        ")";
}

std::string AstFunctionParameterNode::to_string()
{
    return name.value;
}

llvm::Value* AstFunctionParameterNode::codegen()
{
    return nullptr;
}

llvm::Value* AstFunctionDefinitionNode::codegen()
{
    return nullptr;
}

bool AstFunctionDefinitionNode::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next() == TokenType::KEYWORD_FN;
}

std::unique_ptr<AstFunctionParameterNode> AstFunctionParameterNode::try_parse(
    const Scope& scope,
    const std::unique_ptr<TokenSet>& tokens)
{
    const auto param_name = Symbol(tokens->expect(TokenType::IDENTIFIER).lexeme);

    tokens->expect(TokenType::COLON);

    std::unique_ptr<AstType> type_ptr = try_parse_type(tokens);

    return std::make_unique<AstFunctionParameterNode>(param_name, std::move(type_ptr));
}

/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDefinitionNode> AstFunctionDefinitionNode::try_parse(
    const Scope& scope,
    const std::unique_ptr<TokenSet>& tokens)
{
    tokens->expect(TokenType::KEYWORD_FN);

    const auto fn_name = Symbol(tokens->expect(TokenType::IDENTIFIER).lexeme);

    tokens->expect(TokenType::LPAREN);
    std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters = {};

    while (tokens->peak_next() != TokenType::RPAREN)
    {
        do
        {
            const auto next = tokens->peak_next();
            auto param = AstFunctionParameterNode::try_parse(scope, tokens);

            if (std::ranges::find_if(parameters, [&](const std::unique_ptr<AstFunctionParameterNode>& p)
            {
                return p->name == param->name;
            }) != parameters.end())
            {
                tokens->except(
                    next,
                    ErrorType::SEMANTIC_ERROR,
                    std::format(
                        "Duplicate parameter name \"{}\" in function definition",
                        param->name.value)
                );
            }

            parameters.push_back(std::move(param));
        }
        while (tokens->skip_optional(TokenType::COMMA));
    }

    tokens->expect(TokenType::RPAREN);
    tokens->expect(TokenType::COLON);

    std::unique_ptr<AstType> return_type = try_parse_type(tokens);

    std::unique_ptr<AstNode> body = AstBlockNode::try_parse_block(scope, tokens);

    return std::make_unique<AstFunctionDefinitionNode>(
        fn_name,
        std::move(parameters),
        std::move(return_type),
        std::move(body)
    );
}
