#include <iostream>

#include "ast/nodes/blocks.h"

#include <sstream>

#include "ast/parser.h"
#include "ast/nodes/function_declaration.h"

using namespace stride::ast;

void AstBlock::resolve_forward_references(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    for (const auto& child : this->children())
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(child.get()))
        {
            synthesisable->resolve_forward_references(scope, module, context, builder);
        }
    }
}

llvm::Value* AstBlock::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    llvm::Value* last_value = nullptr;

    for (const auto& child : this->children())
    {
        // Don't generate unreachable code, unless it's a function declaration
        // (which defines a new code block / function)
        if (auto* block = builder->GetInsertBlock(); block && block->getTerminator())
        {
            if (dynamic_cast<AstFunctionDeclaration*>(child.get()) == nullptr)
            {
                continue;
            }
        }

        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(child.get()))
        {
            last_value = synthesisable->codegen(this->get_registry(), module, context, builder);
        }
    }

    return last_value;
}

std::string AstBlock::to_string()
{
    std::ostringstream result;
    result << "AstBlock";
    for (const auto& child : this->children())
    {
        result << "\n  " << child->to_string();
    }
    return result.str();
}

std::optional<TokenSet> stride::ast::collect_until_token(TokenSet& set, const TokenType token)
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

            return set.create_subset(initial_offset, relative_offset);
        }
    }
    return std::nullopt;
}

std::optional<TokenSet> stride::ast::collect_block_variant(
    TokenSet& set, const TokenType start_token,
    const TokenType end_token
)
{
    set.expect(start_token);
    for (int64_t level = 1, offset = 0; level > 0 && offset < set.size(); ++offset)
    {
        if (const auto next = set.peak(offset); next.get_type() == start_token)
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
    set.skip(-1); // Ensure we don't point to a random next token that's unrelated
    set.throw_error(std::format("Unmatched closing '{}' found", token_type_to_str(end_token)));
}

std::optional<TokenSet> stride::ast::collect_block(TokenSet& set)
{
    return collect_block_variant(set, TokenType::LBRACE, TokenType::RBRACE);
}

std::unique_ptr<AstBlock> stride::ast::parse_block(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set)
{
    auto collected_subset = collect_block(set);

    if (!collected_subset.has_value())
    {
        return nullptr;
    }

    return parse_sequential(scope, collected_subset.value());
}
