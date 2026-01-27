#include "ast/nodes/for_loop.h"

using namespace stride::ast;

bool stride::ast::is_for_loop_statement(const TokenSet& set)
{
    return set.peak_next_eq(TokenType::KEYWORD_FOR);
}

std::unique_ptr<AstExpression> try_collect_initiator(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set)
{
    auto initiator = collect_until_token(set, TokenType::SEMICOLON);

    if (!initiator.has_value())
    {
        return nullptr;
    }

    return parse_expression_extended(
        SRFLAG_EXPR_TYPE_INLINE,
        scope,
        initiator.value()
    );
}

std::unique_ptr<AstExpression> try_collect_condition(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set)
{
    auto condition = collect_until_token(set, TokenType::SEMICOLON);

    if (!condition.has_value())
    {
        return nullptr;
    }


    return parse_standalone_expression(scope, condition.value());
}

std::unique_ptr<AstExpression> try_collect_incrementor(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set)
{
    if (!set.has_next()) return nullptr; // If there's no incrementor statement, we don't need to parse it.

    return parse_standalone_expression(scope, set);
}

std::unique_ptr<AstForLoop> stride::ast::parse_for_loop_statement(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_FOR);
    const auto header_body_opt = collect_parenthesized_block(set);

    if (!header_body_opt.has_value())
    {
        set.throw_error("Expected for loop header body");
    }

    auto header_body = header_body_opt.value();
    const auto for_scope = std::make_shared<SymbolRegistry>(scope, ScopeType::BLOCK);

    // We can potentially parse a for (<identifier> .. <identifier> { ... }


    auto initiator = try_collect_initiator(for_scope, header_body);
    auto condition = try_collect_condition(for_scope, header_body);
    auto increment = try_collect_incrementor(for_scope, header_body);

    auto body = parse_block(for_scope, set);

    return std::make_unique<AstForLoop>(
        set.source(),
        reference_token.offset,
        for_scope,
        std::move(initiator),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

llvm::Value* AstForLoop::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* loop_cond_bb = llvm::BasicBlock::Create(context, "loop.cond", function);
    llvm::BasicBlock* loop_body_bb = llvm::BasicBlock::Create(context, "loop.body", function);
    llvm::BasicBlock* loop_incr_bb = llvm::BasicBlock::Create(context, "loop.incr", function);
    llvm::BasicBlock* loop_end_bb = llvm::BasicBlock::Create(context, "loop.end", function);

    if (this->get_initializer())
    {
        this->get_initializer()->codegen(scope, module, context, builder);
    }

    builder->CreateBr(loop_cond_bb);
    builder->SetInsertPoint(loop_cond_bb);

    llvm::Value* condValue = nullptr;
    if (const auto cond = this->get_condition(); cond != nullptr)
    {
        condValue = this->get_condition()->codegen(scope, module, context, builder);

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
        this->body()->codegen(scope, module, context, builder);
    }
    builder->CreateBr(loop_incr_bb);

    builder->SetInsertPoint(loop_incr_bb);
    if (get_incrementor())
    {
        this->get_incrementor()->codegen(scope, module, context, builder);
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

    if (this->body() != nullptr) this->body()->validate();
}


std::string AstForLoop::to_string()
{
    return std::format(
        "ForLoop(init: {}, cond: {}, incr: {}, body: {})",
        get_initializer() ? get_initializer()->to_string() : "<empty>",
        get_condition() ? get_condition()->to_string() : "<empty>",
        get_incrementor() ? get_incrementor()->to_string() : "<empty>",
        body() ? body()->to_string() : "<empty>"
    );
}