#include "errors.h"
#include "ast/parsing_context.h"
#include "ast/nodes/control_flow_statements.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;

std::unique_ptr<AstBreakStatement> stride::ast::parse_break_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto& reference_token = set.expect(TokenType::KEYWORD_BREAK);
    set.expect(TokenType::SEMICOLON, "Expected ';' after 'break' statement");

    return std::make_unique<AstBreakStatement>(reference_token.get_source_fragment(), context);
}

llvm::Value* AstBreakStatement::codegen(llvm::Module* module, llvm::IRBuilder<>* builder)
{
    if (ParsingContext::get_control_flow_blocks().empty())
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Break statement outside of loop or switch",
            this->get_source_fragment()
        );
    }

    llvm::BasicBlock* current = builder->GetInsertBlock();
    if (!current)
    {
        return nullptr;
    }

    // If this block is already terminated (e.g. previous break/continue/return),
    // don't emit another terminator.
    if (current->getTerminator())
    {
        return nullptr;
    }

    llvm::BasicBlock* break_target = this->get_context()->get_control_flow_blocks().back().second;
    if (!break_target)
    {
        return nullptr;
    }

    builder->CreateBr(break_target);

    // Move insertion point to a fresh "unreachable" block so subsequent statements
    // in the same source block don't try to emit instructions into a terminated block.
    llvm::Function* fn = current->getParent();
    llvm::BasicBlock* after_break =
        llvm::BasicBlock::Create(module->getContext(), "afterbreak", fn);
    builder->SetInsertPoint(after_break);

    return nullptr;
}

void AstBreakStatement::validate()
{
    if (this->get_context()->get_context_type() != definition::ContextType::CONTROL_FLOW)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            "Break statement cannot appear outside of a loop or switch statement",
            this->get_source_fragment()
        );
    }
}
