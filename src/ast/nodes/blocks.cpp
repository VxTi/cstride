#include "ast/nodes/blocks.h"

#include <iostream>

#include "ast/nodes/enumerables.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/return.h"
#include "ast/nodes/struct.h"

using namespace stride::ast;

std::unique_ptr<IAstNode> parse_next_statement(const Scope& scope, TokenSet& tokens)
{
    if (is_module_statement(tokens))
    {
        return parse_module_statement(scope, tokens);
    }

    if (is_import_statement(tokens))
    {
        return parse_import_statement(scope, tokens);
    }

    if (is_fn_declaration(tokens))
    {
        return parse_fn_declaration(scope, tokens);
    }

    if (is_struct_declaration(tokens))
    {
        return parse_struct_declaration(scope, tokens);
    }


    if (is_enumerable_declaration(tokens))
    {
        return parse_enumerable_declaration(scope, tokens);
    }

    if (is_return_statement(tokens))
    {
        return parse_return_statement(scope, tokens);
    }

    return try_parse_expression(scope, tokens);
}

std::unique_ptr<AstBlock> stride::ast::parse_sequential(const Scope& scope, TokenSet& tokens)
{
    std::vector<std::unique_ptr<IAstNode>> nodes = {};

    while (tokens.has_next())
    {
        if (TokenSet::should_skip_token(tokens.peak_next().type))
        {
            tokens.next();
            continue;
        }

        if (auto result = parse_next_statement(scope, tokens))
        {
            nodes.push_back(std::move(result));
        }
    }

    return std::make_unique<AstBlock>(AstBlock(std::move(nodes)));
}

llvm::Value* AstBlock::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    llvm::Value* last_value = nullptr;

    for (const auto& child : children())
    {
        if (auto* block = builder->GetInsertBlock(); block && block->getTerminator())
        {
            break;
        }

        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(child.get()))
        {
            last_value = synthesisable->codegen(module, context, builder);
        }
    }

    return last_value;
}

std::string AstBlock::to_string()
{
    std::ostringstream result;
    result << "AstBlock";
    for (const auto& child : children())
    {
        result << "\n  " << child->to_string();
    }
    return result.str();
}

std::optional<TokenSet> stride::ast::collect_token_subset(
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

std::optional<TokenSet> stride::ast::collect_block(TokenSet& set)
{
    return collect_token_subset(set, TokenType::LBRACE, TokenType::RBRACE);
}

std::unique_ptr<AstBlock> stride::ast::parse_block(const Scope& scope, TokenSet& set)
{
    auto collected_subset = collect_block(set);

    if (!collected_subset.has_value())
    {
        return nullptr;
    }

    return parse_sequential(scope, collected_subset.value());
}
