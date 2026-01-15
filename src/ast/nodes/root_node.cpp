#include "ast/nodes/root_node.h"

#include <iostream>

#include "ast/nodes/expression.h"
#include "ast/nodes/signature.h"
#include "ast/nodes/function_invocation.h"
#include "ast/nodes/import.h"

using namespace stride::ast;

bool should_skip_token(TokenType type)
{
    switch (type)
    {
    case TokenType::COMMENT:
    case TokenType::COMMENT_MULTILINE:
    case TokenType::END_OF_FILE: return true;
    default: return false;
    }
}

std::unique_ptr<AstNode> AstBlockNode::try_parse(Scope, const std::unique_ptr<TokenSet>& tokens)
{
    std::vector<std::unique_ptr<AstNode>> nodes = {};

    while (tokens->has_next())
    {
        if (should_skip_token(tokens->peak_next().type))
        {
            tokens->next();
            continue;
        }


        if (AstImportNode::can_parse(*tokens))
        {
            nodes.push_back(AstImportNode::try_parse(scope, *tokens));
        }
        else if (AstFunctionDefinitionNode::can_parse(*tokens))
        {
            nodes.push_back(AstFunctionDefinitionNode::try_parse(scope, tokens));
        }
        else if (AstFunctionInvocation::can_parse(*tokens))
        {
            nodes.push_back(AstFunctionInvocation::try_parse(scope, *tokens));
        }
        else if (AstExpression::can_parse(*tokens))
        {
            nodes.push_back(AstExpression::try_parse(scope, *tokens));
        }
        else
        {
            tokens->except("Unexpected token");
        }
    }

    return std::make_unique<AstBlockNode>(AstBlockNode(std::move(nodes)));
}

llvm::Value* AstBlockNode::codegen()
{
    return nullptr;
}

std::string AstBlockNode::to_string()
{
    std::ostringstream result;
    result << "AstBlock";
    for (const auto& child : children)
    {
        result << "\n" << child->to_string();
    }
    return result.str();
}

std::unique_ptr<AstNode> AstBlockNode::try_parse_block(const Scope& scope, const std::unique_ptr<TokenSet>& tokens)
{
    tokens->expect(TokenType::LBRACE);

    for (int64_t level = 1, offset = 0; level > 0 && offset < tokens->size(); ++offset)
    {
        if (const auto next = tokens->peak(offset); next.type == TokenType::LBRACE)
        {
            level++;
        }
        else if (next.type == TokenType::RBRACE)
        {
            level--;
        }

        if (level == 0)
        {
            const auto final_pos = tokens->position();

            tokens->skip(offset + 1);

            // It might happen that the block has a length of 0, e.g., `{}`, therefore we won't
            // parse anything. (offset 0)
            if (offset == 0)
            {
                return nullptr;
            }

            const auto block = tokens->create_subset(final_pos, offset);


            return try_parse(scope, block);
        }
    }
    tokens->except("Unmatched closing bracket");
}
