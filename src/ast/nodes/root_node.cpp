#include "ast/nodes/root_node.h"

#include <iostream>

#include "ast/nodes/enumerables.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_signature.h"
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

std::unique_ptr<AstNode> try_parse_partial(const Scope& scope, TokenSet& tokens)
{
    if (AstImportNode::can_parse(tokens))
    {
        return AstImportNode::try_parse(scope, tokens);
    }

    if (AstFunctionDefinitionNode::can_parse(tokens))
    {
        return AstFunctionDefinitionNode::try_parse(scope, tokens);
    }
    if (AstFunctionInvocation::can_parse(tokens))
    {
        return AstFunctionInvocation::try_parse(scope, tokens);
    }
    if (AstExpression::can_parse(tokens))
    {
        return AstExpression::try_parse(scope, tokens);
    }
    if (AstEnumerable::can_parse(tokens))
    {
        return AstEnumerable::try_parse(scope, tokens);
    }

    tokens.except("Unexpected token");
}

std::unique_ptr<AstNode> AstBlockNode::try_parse(const Scope& scope, TokenSet& tokens)
{
    std::vector<std::unique_ptr<AstNode>> nodes = {};

    while (tokens.has_next())
    {
        if (should_skip_token(tokens.peak_next().type))
        {
            tokens.next();
            continue;
        }

        if (auto result = try_parse_partial(scope, tokens))
        {
            nodes.push_back(std::move(result));
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

std::optional<TokenSet> AstBlockNode::collect_block(TokenSet& set)
{
    set.expect(TokenType::LBRACE);

    for (int64_t level = 1, offset = 0; level > 0 && offset < set.size(); ++offset)
    {
        if (const auto next = set.peak(offset); next.type == TokenType::LBRACE)
        {
            level++;
        }
        else if (next.type == TokenType::RBRACE)
        {
            level--;
        }

        if (level == 0)
        {
            const auto final_pos = set.position();

            set.skip(offset + 1);

            // It might happen that the block has a length of 0, e.g., `{}`, therefore, we won't
            // parse anything. (offset 0)
            if (offset == 0)
            {
                return std::nullopt;
            }

            auto block = set.create_subset(final_pos, offset);

            return block;
        }
    }
    set.except("Unmatched closing bracket");
}

std::unique_ptr<AstNode> AstBlockNode::try_parse_block(const Scope& scope, TokenSet& set)
{
    auto collected_subset = collect_block(set);

    if (!collected_subset.has_value())
    {
        return nullptr;
    }

    return try_parse(scope, collected_subset.value());
}
