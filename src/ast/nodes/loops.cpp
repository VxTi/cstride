#include "ast/nodes/loops.h"

#include "ast/parser.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

bool stride::ast::is_while_loop_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_WHILE);
}

bool stride::ast::is_for_loop_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_FOR);
}

std::unique_ptr<AstExpression> try_collect_initiator(const Scope& scope, TokenSet& set)
{
    auto initiator = collect_until_token(set, TokenType::SEMICOLON);

    if (!initiator.has_value())
    {
        return nullptr;
    }

    return parse_expression_ext(
        SRFLAG_EXPR_INLINE_VARIABLE_DECLARATION |
        SRFLAG_EXPR_ALLOW_VARIABLE_DECLARATION,
        scope,
        initiator.value()
    );
}


std::unique_ptr<AstExpression> try_collect_condition(const Scope& scope, TokenSet& set)
{
    auto condition = collect_until_token(set, TokenType::SEMICOLON);

    if (!condition.has_value())
    {
        return nullptr;
    }


    return parse_standalone_expression(scope, condition.value());
}

std::unique_ptr<AstLoop> stride::ast::parse_for_loop_statement(const Scope& scope, TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_FOR);
    const auto header_body_opt = collect_parenthesized_block(set);

    if (!header_body_opt.has_value())
    {
        set.throw_error("Expected for loop header body");
    }

    auto header_body = header_body_opt.value();
    const auto for_scope = Scope(scope, ScopeType::BLOCK);


    auto body = parse_block(for_scope, set);

    auto initiator = try_collect_initiator(for_scope, header_body);
    auto condition = try_collect_condition(for_scope, header_body);
    auto increment = parse_standalone_expression(for_scope, header_body);

    return std::make_unique<AstLoop>(
        set.source(),
        reference_token.offset,
        std::move(initiator),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

std::unique_ptr<AstLoop> stride::ast::parse_while_loop_statement(const Scope& scope, TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_WHILE);
    const auto header_condition_opt = collect_parenthesized_block(set);

    if (!header_condition_opt.has_value())
    {
        set.throw_error("Expected while loop condition");
    }

    auto header_condition = header_condition_opt.value();

    auto condition = parse_standalone_expression(scope, header_condition);
    auto body = parse_block(scope, set);

    return std::make_unique<AstLoop>(
        set.source(),
        reference_token.offset,
        nullptr,
        std::move(condition),
        nullptr,
        std::move(body)
    );
}

llvm::Value* AstLoop::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb = llvm::BasicBlock::Create(context, "loop.cond", function);
    llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(context, "loop.body", function);
    llvm::BasicBlock* loop_incr_bb = llvm::BasicBlock::Create(context, "loop.incr", function);
    llvm::BasicBlock* loop_end_bb = llvm::BasicBlock::Create(context, "loop.end", function);

    if (this->get_initializer())
    {
        this->get_initializer()->codegen(module, context, builder);
    }

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = nullptr;
    if (const auto cond = this->get_condition(); cond != nullptr)
    {
        condValue = this->get_condition()->codegen(module, context, builder);

        if (condValue == nullptr)
        {
            throw parsing_error(
                make_ast_error(
                    *this->source,
                    this->source_offset,
                    "Failed to codegen loop condition"
                )
            );
        }
    }
    else
    {
        // If no condition is provided, default to true (infinite loop)
        condValue = llvm::ConstantInt::get(context, llvm::APInt(1, 1));
    }

    builder->CreateCondBr(condValue, loop_body_bb, loop_end_bb);

    builder->SetInsertPoint(loop_body_bb);
    if (this->body())
    {
        this->body()->codegen(module, context, builder);
    }
    builder->CreateBr(loop_incr_bb);

    builder->SetInsertPoint(loop_incr_bb);
    if (get_incrementor())
    {
        this->get_incrementor()->codegen(module, context, builder);
    }
    builder->CreateBr(loop_cond_bb);

    builder->SetInsertPoint(loop_end_bb);

    return nullptr;
}

std::string AstLoop::to_string()
{
    std::string result = "Loop(";
    if (get_initializer())
    {
        result += "init: " + get_initializer()->to_string() + ", ";
    }
    if (get_condition())
    {
        result += "cond: " + get_condition()->to_string() + ", ";
    }
    if (get_incrementor())
    {
        result += "incr: " + get_incrementor()->to_string() + ", ";
    }
    if (body())
    {
        result += "body: " + body()->to_string();
    }
    result += ")";
    return result;
}
