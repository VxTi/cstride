#include "ast/nodes/blocks.h"

#include "ast/ast.h"
#include "ast/nodes/function_declaration.h"
#include "ast/tokens/token_set.h"

#include <iostream>
#include <sstream>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::unique_ptr<AstBlock> stride::ast::parse_block(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    auto collected_subset = collect_block(set);

    if (!collected_subset.has_value())
    {
        return AstBlock::create_empty(context, set.peek_next().get_source_fragment());
    }

    return parse_sequential(context, collected_subset.value());
}

std::optional<TokenSet> stride::ast::collect_until_token(
    TokenSet& set,
    const TokenType token)
{
    const size_t initial_offset = set.position();

    for (int64_t relative_offset = 0; set.has_next(); relative_offset++)
    {
        if (const auto next = set.next(); next.get_type() == token)
        {
            // If we immediately find the requested token,
            // then there's no point in creating a new subset
            if (relative_offset == 0)
            {
                return std::nullopt;
            }

            return set.create_subset(static_cast<int64_t>(initial_offset), relative_offset);
        }
    }
    return std::nullopt;
}

std::optional<TokenSet> stride::ast::collect_block_variant(
    TokenSet& set,
    const TokenType start_token,
    const TokenType end_token)
{
    set.expect(start_token);
    for (int64_t level = 1, offset = 0; level > 0 && offset < set.size(); ++
         offset)
    {
        if (const auto next = set.peek(offset); next.get_type() == start_token)
        {
            level++;
        }
        else if (next.get_type() == end_token)
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
    // Ensure we don't point to a random next token that's unrelated
    set.skip(-1);
    set.throw_error(std::format("Unmatched closing '{}' found", token_type_to_str(end_token)));
}

std::optional<TokenSet> stride::ast::collect_block(TokenSet& set)
{
    return collect_block_variant(set, TokenType::LBRACE, TokenType::RBRACE);
}

std::optional<TokenSet> stride::ast::collect_parenthesized_block(TokenSet& set)
{
    return collect_block_variant(set, TokenType::LPAREN, TokenType::RPAREN);
}

void AstBlock::aggregate_block(AstBlock* other)
{
    for (auto& child : other->_children)
    {
        this->_children.push_back(std::move(child));
    }
}

void AstBlock::resolve_forward_references(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    for (const auto& child : this->_children)
    {
        child->resolve_forward_references(module, builder);
    }
}

llvm::Value* AstBlock::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    llvm::Value* last_value = nullptr;

    for (const auto& child : this->_children)
    {
        last_value = child->codegen(module, builder);
    }

    return last_value;
}

void AstBlock::validate()
{
    for (const auto& child : this->_children)
    {
        child->validate();
    }
}

std::unique_ptr<IAstNode> AstBlock::clone()
{
    std::vector<std::unique_ptr<IAstNode>> cloned_children;
    cloned_children.reserve(this->get_children().size());

    for (const auto& child : this->get_children())
    {
        cloned_children.push_back(child->clone());
    }

    return std::make_unique<AstBlock>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(cloned_children)
    );
}

std::string AstBlock::to_string()
{
    std::ostringstream result;
    result << "AstBlock";
    for (const auto& child : this->get_children())
    {
        result << "\n  " << child->to_string();
    }
    return result.str();
}
