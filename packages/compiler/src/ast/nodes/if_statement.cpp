#include <memory>

#include "ast/nodes/if_statement.h"

#include "ast/flags.h"
#include "ast/parser.h"

using namespace stride::ast;

std::unique_ptr<AstBlock> parse_else_optional(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set)
{
    if (!set.peak_next_eq(TokenType::KEYWORD_ELSE))
    {
        return nullptr;
    }

    const auto reference_token = set.next();

    // In case we encounter a `{` after an `if ... else {`, we'll
    // have to parse_file the block separately
    if (set.peak_next_eq(TokenType::LBRACE))
    {
        return parse_block(scope, set);
    }

    std::vector<std::unique_ptr<IAstNode>> nodes;
    nodes.push_back(parse_next_statement(scope, set));

    // Otherwise, we can just parse_file it as a next statement.
    return std::make_unique<AstBlock>(
        set.source(),
        reference_token.offset,
        scope,
        std::move(nodes)
    );
}

std::unique_ptr<AstIfStatement> stride::ast::parse_if_statement(const std::shared_ptr<SymbolRegistry>& scope,
                                                                TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_IF);

    auto if_header_scope = std::make_shared<SymbolRegistry>(scope, ScopeType::BLOCK);
    auto if_header_body = collect_parenthesized_block(set);

    if (!if_header_body.has_value())
    {
        set.throw_error("Expected condition block after 'if' keyword");
    }

    auto condition = parse_expression_extended(
        0,
        if_header_scope,
        if_header_body.value()
    );

    auto body = parse_block(if_header_scope, set);

    auto else_statement = parse_else_optional(scope, set);

    return std::make_unique<AstIfStatement>(
        set.source(),
        reference_token.offset,
        scope,
        std::move(condition),
        std::move(body),
        std::move(else_statement)
    );
}

bool stride::ast::is_if_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_IF);
}

IAstNode* AstIfStatement::reduce()
{
    return this;
}

bool AstIfStatement::is_reducible()
{
    return this->get_condition()->is_reducible();
}

llvm::Value* AstIfStatement::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                                     llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    if (this->get_condition() == nullptr)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "If statement condition is empty"
            )
        );
    }

    if (this->get_block() == nullptr)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "If statement body is empty"
            )
        );
    }

    // 1. Generate Condition
    llvm::Value* cond_value = this->get_condition()->codegen(scope, module, context, builder);

    if (cond_value == nullptr)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Unable to generate condition value"
            )
        );
    }

    llvm::Function* parent_function = builder->GetInsertBlock()->getParent();

    // 2. Create Blocks
    llvm::BasicBlock* then_body_bb = llvm::BasicBlock::Create(context, "then_body", parent_function);
    llvm::BasicBlock* else_body_bb = this->get_else_block() != nullptr
                                         ? llvm::BasicBlock::Create(context, "else_body", parent_function)
                                         : nullptr;
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(context, "if_merge", parent_function);

    // 3. Create Conditional Branch
    builder->CreateCondBr(cond_value, then_body_bb, else_body_bb != nullptr ? else_body_bb : merge_bb);

    // 4. Generate 'Then' Block
    builder->SetInsertPoint(then_body_bb);
    this->get_block()->codegen(scope, module, context, builder);

    // Only create a branch to the merge block if the current block
    // does not already have a terminator (like a 'ret' or 'break').
    if (builder->GetInsertBlock()->getTerminator() == nullptr)
    {
        builder->CreateBr(merge_bb);
    }
    // 5. Generate 'Else' Block
    if (else_body_bb != nullptr)
    {
        builder->SetInsertPoint(else_body_bb);
        this->get_else_block()->codegen(scope, module, context, builder);

        // Same check for the else block
        if (builder->GetInsertBlock()->getTerminator() == nullptr)
        {
            builder->CreateBr(merge_bb);
        }
    }

    // 6. Continue merging
    builder->SetInsertPoint(merge_bb);

    // Optional: If both paths return early, merge_bb might have no predecessors.
    // If you care about clean IR immediately, you might check if merge_bb has uses,
    // but LLVM optimization passes (DCE) will usually clean up unreachable blocks for you.

    return nullptr;
}

void AstIfStatement::validate() {}

std::string AstIfStatement::to_string()
{
    return std::format(
        "IfStatement({}) {} {}",
        this->get_condition() != nullptr ? this->get_condition()->to_string() : "<empty>",
        this->get_block() != nullptr ? this->get_block()->to_string() : "<empty>",
        this->get_else_block() != nullptr ? get_else_block()->to_string() : ""
    );
}
