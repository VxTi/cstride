#include "ast/nodes/while_loop.h"
#include "ast/parser.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

bool stride::ast::is_while_loop_statement(const TokenSet& set)
{
    return set.peak_next_eq(TokenType::KEYWORD_WHILE);
}

void AstWhileLoop::validate()
{
    if (this->_condition != nullptr) this->_condition->validate();

    if (this->_body != nullptr) this->_body->validate();
}

std::unique_ptr<AstWhileLoop> stride::ast::parse_while_loop_statement(
    const std::shared_ptr<SymbolRegistry>& scope,
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

    auto condition = parse_standalone_expression(scope, header_condition);
    auto body = parse_block(scope, set);

    return std::make_unique<AstWhileLoop>(
        set.get_source(),
        reference_token.get_source_position(),
        scope,
        std::move(condition),
        std::move(body)
    );
}

llvm::Value* AstWhileLoop::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                                   llvm::LLVMContext& context,
                                   llvm::IRBuilder<>* builder)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb = llvm::BasicBlock::Create(context, "loop.cond", function);
    llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(context, "loop.body", function);
    llvm::BasicBlock* loop_end_bb = llvm::BasicBlock::Create(context, "loop.end", function);

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = nullptr;
    if (const auto cond = this->get_condition(); cond != nullptr)
    {
        condValue = this->get_condition()->codegen(scope, module, context, builder);

        if (condValue == nullptr)
        {
            throw parsing_error(
                make_source_error(
                    *this->get_source(),
                    ErrorType::RUNTIME_ERROR,
                    "Failed to codegen loop condition",
                    this->get_source_position()
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
        this->body()->codegen(scope, module, context, builder);
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
