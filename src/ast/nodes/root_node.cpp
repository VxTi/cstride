#include "ast/nodes/root_node.h"

#include <iostream>

#include "ast/nodes/function_definition.h"
#include "ast/nodes/import_node.h"

using namespace stride::ast;

std::unique_ptr<AstNode> AstBlockNode::try_parse(const Scope& scope, const std::unique_ptr<TokenSet>& tokens)
{
    std::cout << "Parsing block\n";
    std::vector<std::unique_ptr<AstNode>> nodes = {};

    while (tokens->has_next())
    {
        if (AstImportNode::can_parse(*tokens))
        {
            nodes.push_back(AstImportNode::try_parse(*tokens));
        }
        else if (AstFunctionDefinitionNode::can_parse(*tokens))
        {
            nodes.push_back(AstFunctionDefinitionNode::try_parse(scope, tokens));
        }
        else
        {
            tokens->except("Unexpected token at root level");
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

    for (long int level = 1, offset = 0; level > 0 && offset < tokens->size(); offset++)
    {
        if (tokens->peak_next().type == TokenType::LBRACE)
        {
            ++level;
        }
        else if (tokens->peak_next().type == TokenType::RBRACE)
        {
            --level;
        }

        if (level == 0)
        {
            const auto block = tokens->copy(tokens->position(), offset - 1);

            tokens->skip(offset);

            return try_parse(scope, block);
        }
    }
    tokens->except("Unmatched closing bracket");
}
