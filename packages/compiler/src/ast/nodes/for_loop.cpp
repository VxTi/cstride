#include "ast/nodes/for_loop.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<AstExpression> collect_initiator(const std::shared_ptr<ParsingContext>& context, TokenSet& set)
{
    auto initiator = collect_until_token(set, TokenType::SEMICOLON);

    if (!initiator.has_value())
    {
        return nullptr;
    }

    return parse_variable_declaration_inline(
        context,
        initiator.value(),
        VisibilityModifier::NONE
    );
}

std::unique_ptr<AstExpression> collect_condition(const std::shared_ptr<ParsingContext>& context, TokenSet& set)
{
    auto condition = collect_until_token(set, TokenType::SEMICOLON);

    if (!condition.has_value())
    {
        return nullptr;
    }

    // This one doesn't allow variable declarations
    return parse_inline_expression(context, condition.value());
}

std::unique_ptr<AstExpression> collect_incrementor(const std::shared_ptr<ParsingContext>& context, TokenSet& set)
{
    if (!set.has_next()) return nullptr; // If there's no incrementor statement, we don't need to parse it.

    return parse_inline_expression(context, set);
}

std::unique_ptr<AstForLoop> stride::ast::parse_for_loop_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_FOR);
    const auto header_body_opt = collect_parenthesized_block(set);

    if (!header_body_opt.has_value())
    {
        set.throw_error("Expected for loop header body");
    }

    auto header_body = header_body_opt.value();
    const auto for_scope = std::make_shared<ParsingContext>(context, ScopeType::BLOCK);

    // We can potentially parse a for (<identifier> .. <identifier> { ... }

    auto initiator = collect_initiator(for_scope, header_body);
    auto condition = collect_condition(for_scope, header_body);
    auto increment = collect_incrementor(for_scope, header_body);

    auto body = parse_block(for_scope, set);

    return std::make_unique<AstForLoop>(
        reference_token.get_source_position(),
        for_scope,
        std::move(initiator),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

llvm::Value* AstForLoop::codegen(
    const ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb = llvm::BasicBlock::Create(module->getContext(), "loop.cond", function);
    llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(module->getContext(), "loop.body", function);
    llvm::BasicBlock* loop_incr_bb = llvm::BasicBlock::Create(module->getContext(), "loop.incr", function);
    llvm::BasicBlock* loop_end_bb = llvm::BasicBlock::Create(module->getContext(), "loop.end", function);

    if (this->get_initializer())
    {
        this->get_initializer()->codegen(context, module, builder);
    }

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = nullptr;
    if (const auto cond = this->get_condition(); cond != nullptr)
    {
        condValue = this->get_condition()->codegen(context, module, builder);

        if (condValue == nullptr)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                "Failed to codegen loop condition",
                this->get_source_position()
            );
        }
    }
    else
    {
        // If no condition is provided, default to true (infinite loop)
        condValue = llvm::ConstantInt::get(module->getContext(), llvm::APInt(1, 1));
    }

    builder->CreateCondBr(condValue, loop_body_bb, loop_end_bb);

    builder->SetInsertPoint(loop_body_bb);
    if (this->get_body())
    {
        this->get_body()->codegen(context, module, builder);
    }
    builder->CreateBr(loop_incr_bb);

    builder->SetInsertPoint(loop_incr_bb);
    if (get_incrementor())
    {
        this->get_incrementor()->codegen(context, module, builder);
    }
    builder->CreateBr(loop_cond_bb);

    builder->SetInsertPoint(loop_end_bb);

    return nullptr;
}

void AstForLoop::validate()
{
    if (this->_initializer != nullptr) this->_initializer->validate();

    if (this->_condition != nullptr) this->_condition->validate();

    if (this->_incrementor != nullptr) this->_incrementor->validate();

    if (this->get_body() != nullptr) this->get_body()->validate();
}


std::string AstForLoop::to_string()
{
    return std::format(
        "ForLoop(init: {}, cond: {}, incr: {}, body: {})",
        get_initializer() ? get_initializer()->to_string() : "<empty>",
        get_condition() ? get_condition()->to_string() : "<empty>",
        get_incrementor() ? get_incrementor()->to_string() : "<empty>",
        get_body() ? get_body()->to_string() : "<empty>"
    );
}
