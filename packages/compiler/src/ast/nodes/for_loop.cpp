#include "ast/nodes/for_loop.h"

#include "ast/conditionals.h"
#include "ast/modifiers.h"
#include "ast/symbol_table.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstExpression> collect_initiator(TokenSet& set)
{
    auto initiator = collect_until_token(set, TokenType::SEMICOLON);

    if (!initiator.has_value())
    {
        return nullptr;
    }

    return parse_variable_declaration_inline(
        initiator.value(),
        VisibilityModifier::PRIVATE // Irrelevant here
    );
}

std::unique_ptr<IAstExpression> collect_condition(TokenSet& set)
{
    auto condition = collect_until_token(set, TokenType::SEMICOLON);

    if (!condition.has_value())
    {
        return nullptr;
    }

    // This one doesn't allow variable declarations
    return parse_inline_expression(condition.value());
}

std::unique_ptr<IAstExpression> collect_incrementor(TokenSet& set)
{
    if (!set.has_next())
        return nullptr;
    // If there's no incrementor statement, we don't need to parse it.

    return parse_inline_expression(set);
}

std::unique_ptr<AstForLoop> stride::ast::parse_for_loop_statement(
    TokenSet& set,
    [[maybe_unused]] VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_FOR);
    const auto header_body_opt = collect_parenthesized_block(set);

    if (!header_body_opt.has_value())
    {
        set.throw_error("Expected for loop header body");
    }

    auto header_body = header_body_opt.value();

    // We can potentially parse a for (<identifier> .. <identifier> { ... }

    auto initiator = collect_initiator(header_body);
    auto condition = collect_condition(header_body);
    auto increment = collect_incrementor(header_body);

    auto body = parse_block(set);

    return std::make_unique<AstForLoop>(
        reference_token.get_source_position(),
        std::move(initiator),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

llvm::Value* AstForLoop::codegen(
    SymbolTable* symbol_table,
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.cond", function);
    llvm::BasicBlock* loop_body_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.body", function);
    llvm::BasicBlock* loop_continue_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.incr", function);
    llvm::BasicBlock* loop_end_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.end", function);

    if (this->get_initializer())
    {
        this->get_initializer()->codegen(symbol_table, module, builder);
    }

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = codegen_conditional_value(symbol_table, module, builder, this->get_condition());

    builder->CreateCondBr(condValue, loop_body_bb, loop_end_bb);
    builder->SetInsertPoint(loop_body_bb);

    if (this->get_body())
    {
        SymbolTable::push_control_flow_block(loop_continue_bb, loop_end_bb);

        this->get_body()->codegen(symbol_table, module, builder);

        SymbolTable::pop_control_flow_block();
    }

    // If we already have a terminator (e.g., from a break or continue),
    // we don't want to add another branch instruction.
    if (!builder->GetInsertBlock()->getTerminator())
    {
        builder->CreateBr(loop_continue_bb);
    }

    builder->SetInsertPoint(loop_continue_bb);

    if (get_incrementor())
    {
        this->get_incrementor()->codegen(symbol_table, module, builder);
    }

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_end_bb);

    return nullptr;
}

void AstForLoop::validate(SymbolTable* symbol_table)
{
    if (this->_initializer)
        this->_initializer->validate(symbol_table);

    if (this->_condition)
        this->_condition->validate(symbol_table);

    if (this->_incrementor)
        this->_incrementor->validate(symbol_table);

    this->_body->validate(symbol_table);
}

std::unique_ptr<IAstNode> AstForLoop::clone()
{
    return std::make_unique<AstForLoop>(
        this->get_source_position(),
        this->_initializer ? this->_initializer->clone_as<IAstExpression>() : nullptr,
        this->_condition ? this->_condition->clone_as<IAstExpression>() : nullptr,
        this->_incrementor ? this->_incrementor->clone_as<IAstExpression>() : nullptr,
        this->_body->clone_as<AstBlock>()
    );
}

std::string AstForLoop::to_string()
{
    return std::format(
        "ForLoop(init: {}, cond: {}, incr: {}, body: {})",
        get_initializer() ? get_initializer()->to_string() : "<empty>",
        get_condition() ? get_condition()->to_string() : "<empty>",
        get_incrementor() ? get_incrementor()->to_string() : "<empty>",
        get_body() ? get_body()->to_string() : "<empty>");
}
