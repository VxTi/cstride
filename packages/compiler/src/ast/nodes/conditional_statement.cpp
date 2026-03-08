#include "ast/nodes/conditional_statement.h"

#include "errors.h"
#include "ast/ast.h"
#include "ast/conditionals.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"

#include <memory>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<AstBlock> parse_else_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (!set.peek_next_eq(TokenType::KEYWORD_ELSE))
    {
        return nullptr;
    }

    const auto reference_token = set.next();
    const auto else_block_context = std::make_shared<ParsingContext>(
        context,
        context->get_context_type()
    );

    // In case we encounter a `{` after an `if ... else {`, we'll
    // have to parse_file the block separately
    if (set.peek_next_eq(TokenType::LBRACE))
    {
        const auto else_context = std::make_shared<ParsingContext>(
            context,
            context->get_context_type());
        return parse_block(else_context, set);
    }

    // Otherwise, we can just parse the next statement, as it will only allow a single one.
    std::vector<std::unique_ptr<IAstNode>> nodes;
    nodes.push_back(parse_next_statement(else_block_context, set));

    return std::make_unique<AstBlock>(
        reference_token.get_source_fragment(),
        else_block_context,
        std::move(nodes)
    );
}

std::unique_ptr<AstConditionalStatement> stride::ast::parse_if_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_IF);

    auto conditional_context = std::make_shared<ParsingContext>(
        context,
        context->get_context_type());
    auto if_header_body = collect_parenthesized_block(set);

    if (!if_header_body.has_value())
    {
        set.throw_error("Expected condition block after 'if' keyword");
    }
    auto condition = parse_inline_expression(conditional_context, if_header_body.value());

    // Thus far, we've collected `if (...)

    if (!set.peek_next_eq(TokenType::LBRACE))
    {
        // Here we might have an `if (...) ...` statement.
        auto if_body_expr = parse_next_statement(conditional_context, set);

        if (if_body_expr == nullptr)
        {
            throw parsing_error(
                ErrorType::SYNTAX_ERROR,
                "Expected condition after 'if' keyword",
                reference_token.get_source_fragment()
            );
        }

        std::vector<std::unique_ptr<IAstNode>> nodes;
        nodes.push_back(std::move(if_body_expr));

        auto if_body = std::make_unique<AstBlock>(
            reference_token.get_source_fragment(),
            conditional_context,
            std::move(nodes)
        );

        // dangling `else`, still possible.
        // This would allow one to write statements like:
        // if ( ... )
        //   <some expression>
        // else
        //   <some other expression>
        auto else_statement = parse_else_optional(conditional_context, set);

        return std::make_unique<AstConditionalStatement>(
            reference_token.get_source_fragment(),
            conditional_context,
            std::move(condition),
            std::move(if_body),
            std::move(else_statement)
        );
    }

    // Now we're parsing an `if (...) { ... }` statement
    auto body = parse_block(conditional_context, set);

    auto else_statement = parse_else_optional(conditional_context, set);

    return std::make_unique<AstConditionalStatement>(
        reference_token.get_source_fragment(),
        context,
        std::move(condition),
        std::move(body),
        std::move(else_statement)
    );
}

std::optional<std::unique_ptr<IAstNode>> AstConditionalStatement::reduce()
{
    return std::nullopt;
}

bool AstConditionalStatement::is_reducible()
{
    return this->get_condition()->is_reducible();
}

llvm::Value* AstConditionalStatement::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    if (!this->_condition)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "If statement condition is empty",
            this->get_source_fragment());
    }

    if (!this->_body)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "If statement body is empty",
            this->get_source_fragment());
    }

    // Generate Condition
    llvm::Value* cond_value = codegen_conditional_value(module, builder, this->_condition.get());

    llvm::Function* parent_function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* then_body_bb = llvm::BasicBlock::Create(
        module->getContext(),
        "then_body",
        parent_function
    );
    llvm::BasicBlock* else_body_bb = this->get_else_body() != nullptr
        ? llvm::BasicBlock::Create(
            module->getContext(),
            "else_body",
            parent_function
        )
        : nullptr;
    llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(
        module->getContext(),
        "if_merge",
        parent_function);

    builder->CreateCondBr(
        cond_value,
        then_body_bb,
        else_body_bb != nullptr ? else_body_bb : merge_bb);
    builder->SetInsertPoint(then_body_bb);

    this->get_body()->codegen(module, builder);

    // Only create a branch to the merge block if the current block
    // does not already have a terminator (like a 'ret' or 'break').
    if (builder->GetInsertBlock()->getTerminator() == nullptr)
    {
        builder->CreateBr(merge_bb);
    }

    if (else_body_bb != nullptr)
    {
        builder->SetInsertPoint(else_body_bb);

        this->_else_body->codegen(module, builder);

        // Same check for the else block
        if (builder->GetInsertBlock()->getTerminator() == nullptr)
        {
            builder->CreateBr(merge_bb);
        }
    }

    builder->SetInsertPoint(merge_bb);

    return nullptr;
}

void AstConditionalStatement::validate()
{
    this->_condition->validate();

    this->_body->validate();

    if (this->_else_body != nullptr)
        this->_else_body->validate();
}

std::unique_ptr<IAstNode> AstConditionalStatement::clone()
{
    return std::make_unique<AstConditionalStatement>(
        this->get_source_fragment(),
        this->get_context(),
        this->_condition->clone_as<IAstExpression>(),
        this->_body->clone_as<AstBlock>(),
        this->_else_body != nullptr ? this->_else_body->clone_as<AstBlock>() : nullptr
    );
}

std::string AstConditionalStatement::to_string()
{
    return std::format(
        "IfStatement({}) {} {}",
        this->get_condition() != nullptr
        ? this->get_condition()->to_string()
        : "<empty>",
        this->get_body() != nullptr ? this->get_body()->to_string() : "<empty>",
        this->get_else_body() != nullptr ? get_else_body()->to_string() : "");
}
