#include "ast/nodes/while_loop.h"

#include <llvm/IR/Module.h>

#include "ast/parser.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

bool stride::ast::is_while_loop_statement(const TokenSet& set)
{
    return set.peek_next_eq(TokenType::KEYWORD_WHILE);
}

void AstWhileLoop::validate()
{
    if (this->_condition != nullptr) this->_condition->validate();

    if (this->_body != nullptr) this->_body->validate();
}

std::unique_ptr<AstWhileLoop> stride::ast::parse_while_loop_statement(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_WHILE);
    const auto header_condition_opt = collect_parenthesized_block(set);

    if (!header_condition_opt.has_value())
    {
        set.throw_error("Expected while loop condition");
    }

    auto header_condition = header_condition_opt.value();

    auto condition = parse_standalone_expression(registry, header_condition);
    auto body = parse_block(registry, set);

    return std::make_unique<AstWhileLoop>(
        set.get_source(),
        reference_token.get_source_position(),
        registry,
        std::move(condition),
        std::move(body)
    );
}

llvm::Value* AstWhileLoop::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb = llvm::BasicBlock::Create(module->getContext(), "loop.cond", function);
    llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(module->getContext(), "loop.body", function);
    llvm::BasicBlock* loop_end_bb = llvm::BasicBlock::Create(module->getContext(), "loop.end", function);

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = nullptr;
    if (const auto cond = this->get_condition(); cond != nullptr)
    {
        condValue = this->get_condition()->codegen(registry, module, builder);

        if (condValue == nullptr)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                "Failed to codegen loop condition",
                *this->get_source(),
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
    if (this->body())
    {
        this->body()->codegen(registry, module, builder);
    }
    builder->CreateBr(loop_cond_bb);

    builder->SetInsertPoint(loop_end_bb);

    return nullptr;
}

std::string AstWhileLoop::to_string()
{
    return std::format(
        "WhileLoop(cond: {}, body: {})",
        get_condition() ? get_condition()->to_string() : "<empty>",
        body() ? body()->to_string() : "<empty>"
    );
}
