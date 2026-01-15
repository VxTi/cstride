#include "ast/nodes/signature.h"

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

    const auto body_str = this->body_ == nullptr ? "EMPTY" : this->body_->to_string();

    return std::format(
        "FunctionDefinition(name: {}, parameters: [ {} ], body: {})",
        this->name_.value,
        params,
        body_str
    );
}


// TODO
llvm::Value* AstFunctionParameterNode::codegen()
{
    return nullptr;
}

// TODO
llvm::Value* AstFunctionDefinitionNode::codegen()
{
    return nullptr;
}

bool AstFunctionDefinitionNode::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_FN);
}

void traverse_subsequent_parameters(
    const Scope& scope,
    const std::unique_ptr<TokenSet>& tokens,
    std::vector<std::unique_ptr<AstFunctionParameterNode>>& parameters
)
{
    while (tokens->peak_next_eq(TokenType::COMMA))
    {
        tokens->expect(TokenType::COMMA);
        const auto next = tokens->peak_next();

        auto param = AstFunctionParameterNode::try_parse(scope, tokens);

        if (std::ranges::find_if(parameters, [&](const std::unique_ptr<AstFunctionParameterNode>& p)
        {
            return p->name == param->name;
        }) != parameters.end())
        {
            tokens->except(
                next,
                stride::ErrorType::SEMANTIC_ERROR,
                std::format(
                    "Duplicate parameter name \"{}\" in function definition",
                    param->name.value)
            );
        }

        parameters.push_back(std::move(param));
    }
}

/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDefinitionNode> AstFunctionDefinitionNode::try_parse(
    const Scope& scope,
    const std::unique_ptr<TokenSet>& tokens
)
{
    tokens->expect(TokenType::KEYWORD_FN);

    const auto fn_name = Symbol(tokens->expect(TokenType::IDENTIFIER).lexeme);
    scope.try_define_symbol(*tokens->source(), tokens->position(), fn_name);

    tokens->expect(TokenType::LPAREN);
    std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters = {};

    while (!tokens->peak_next_eq(TokenType::RPAREN))
    {
        auto initial = AstFunctionParameterNode::try_parse(scope, tokens);
        parameters.push_back(std::move(initial));

        traverse_subsequent_parameters(scope, tokens, parameters);
    }

    tokens->expect(TokenType::RPAREN);
    tokens->expect(TokenType::COLON);

    const Scope function_scope(scope, ScopeType::FUNCTION);

    std::unique_ptr<AstType> return_type = try_parse_type(tokens);
    std::unique_ptr<AstNode> body = AstBlockNode::try_parse_block(function_scope, tokens);

    return std::make_unique<AstFunctionDefinitionNode>(
        fn_name,
        std::move(parameters),
        std::move(return_type),
        std::move(body)
    );
}
