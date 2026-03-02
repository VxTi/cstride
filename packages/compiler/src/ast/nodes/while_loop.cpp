#include "ast/nodes/while_loop.h"

#include "ast/conditionals.h"
#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

void AstWhileLoop::validate()
{
    if (this->_condition != nullptr)
        this->_condition->validate();

    this->_body->validate();
}

std::unique_ptr<AstWhileLoop> stride::ast::parse_while_loop_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    [[maybe_unused]] VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_WHILE);
    const auto header_condition_opt = collect_parenthesized_block(set);

    if (!header_condition_opt.has_value())
    {
        set.throw_error("Expected while loop condition");
    }

    const auto while_body_context = std::make_shared<ParsingContext>(
        context,
        definition::ContextType::CONTROL_FLOW);
    auto header_condition = header_condition_opt.value();

    auto condition = parse_inline_expression(while_body_context, header_condition);
    auto body = parse_block(while_body_context, set);

    return std::make_unique<AstWhileLoop>(
        reference_token.get_source_fragment(),
        while_body_context,
        std::move(condition),
        std::move(body)
    );
}

llvm::Value* AstWhileLoop::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.cond", function);
    llvm::BasicBlock* loop_body_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.body", function);
    llvm::BasicBlock* loop_end_bb =
        llvm::BasicBlock::Create(module->getContext(), "loop.end", function);

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = codegen_conditional_value(module, builder, this->get_condition());

    builder->CreateCondBr(condValue, loop_body_bb, loop_end_bb);
    builder->SetInsertPoint(loop_body_bb);

    if (this->get_body())
    {
        ParsingContext::push_control_flow_block(loop_cond_bb, loop_end_bb);

        this->get_body()->codegen(module, builder);

        ParsingContext::pop_control_flow_block();
    }

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_end_bb);

    return nullptr;
}

void AstWhileLoop::resolve_types()
{
    if (this->_condition != nullptr)
        this->_condition->resolve_types();

    this->_body->resolve_types();
}

std::string AstWhileLoop::to_string()
{
    return std::format(
        "WhileLoop(cond: {}, body: {})",
        get_condition() ? get_condition()->to_string() : "<empty>",
        get_body() ? get_body()->to_string() : "<empty>"
    );
}

std::unique_ptr<IAstNode> AstWhileLoop::clone()
{
    return std::make_unique<AstWhileLoop>(
        this->get_source_fragment(),
        this->get_context(),
        this->_condition->clone_as<IAstExpression>(),
        this->_body->clone_as<AstBlock>()
    );
}
