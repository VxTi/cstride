#include "ast/nodes/blocks.h"

#include <iostream>

#include "ast/nodes/enumerables.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_signature.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/struct.h"

using namespace stride::ast;

std::unique_ptr<IAstNode> try_parse_partial(const Scope& scope, TokenSet& tokens)
{
    if (AstModule::can_parse(tokens))
    {
        return AstModule::try_parse(scope, tokens);
    }

    if (AstImportNode::can_parse(tokens))
    {
        return AstImportNode::try_parse(scope, tokens);
    }

    if (AstFunctionDefinitionNode::can_parse(tokens))
    {
        return AstFunctionDefinitionNode::try_parse(scope, tokens);
    }

    if (AstStruct::can_parse(tokens))
    {
        return AstStruct::try_parse(scope, tokens);
    }

    if (AstEnumerable::can_parse(tokens))
    {
        return AstEnumerable::try_parse(scope, tokens);
    }

    return try_parse_expression(scope, tokens);
}

std::unique_ptr<AstBlockNode> AstBlockNode::try_parse(const Scope& scope, TokenSet& tokens)
{
    std::vector<std::unique_ptr<IAstNode>> nodes = {};

    while (tokens.has_next())
    {
        if (TokenSet::should_skip_token(tokens.peak_next().type))
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

llvm::Value* AstBlockNode::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    return nullptr;
}

std::string AstBlockNode::to_string()
{
    std::ostringstream result;
    result << "AstBlock";
    for (const auto& child : children())
    {
        result << "\n" << child->to_string();
    }
    return result.str();
}

std::optional<TokenSet> AstBlockNode::collect_block_until(
    TokenSet& set, const TokenType start_token,
    const TokenType end_token
)
{
    set.expect(start_token);
    for (int64_t level = 1, offset = 0; level > 0 && offset < set.size(); ++offset)
    {
        if (const auto next = set.peak(offset); next.type == start_token)
        {
            level++;
        }
        else if (next.type == end_token)
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
    set.skip(-1); // Ensure we don't point to a random next token that's unrelated
    set.throw_error(std::format("Unmatched closing {}", token_type_to_str(end_token)));
}

std::optional<TokenSet> AstBlockNode::collect_block(TokenSet& set)
{
    return collect_block_until(set, TokenType::LBRACE, TokenType::RBRACE);
}

std::unique_ptr<AstBlockNode> AstBlockNode::try_parse_block(const Scope& scope, TokenSet& set)
{
    auto collected_subset = collect_block(set);

    if (!collected_subset.has_value())
    {
        return nullptr;
    }

    return try_parse(scope, collected_subset.value());
}
