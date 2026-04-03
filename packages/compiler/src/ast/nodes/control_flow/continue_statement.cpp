#include "errors.h"
#include "ast/symbol_table.h"
#include "ast/nodes/control_flow_statements.h"

#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::unique_ptr<AstContinueStatement> stride::ast::parse_continue_statement(TokenSet& set)
{
    const auto& reference_token = set.expect(TokenType::KEYWORD_CONTINUE);
    set.expect(TokenType::SEMICOLON, "Expected ';' after 'continue' statement");

    return std::make_unique<AstContinueStatement>(reference_token.get_source_position());
}

llvm::Value* AstContinueStatement::codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder)
{
    if (SymbolTable::get_control_flow_blocks().empty())
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            "Continue statement outside of loop",
            this->get_source_position()
        );
    }

    const auto continue_block = SymbolTable::get_control_flow_blocks().back().first;
    builder->CreateBr(continue_block);

    // Since we branched, create a new block for unreachable code, but since it's a statement, return nullptr
    return nullptr;
}

void AstContinueStatement::validate(SymbolTable* symbol_table)
{
    if (symbol_table->get_context_type() != ContextType::CONTROL_FLOW)
    {
        throw stride_error(
            ErrorType::SYNTAX_ERROR,
            "Continue statement cannot appear outside of a loop or switch statement",
            this->get_source_position()
        );
    }
}

std::unique_ptr<IAstNode> AstContinueStatement::clone()
{
    return std::make_unique<AstContinueStatement>(*this);
}
