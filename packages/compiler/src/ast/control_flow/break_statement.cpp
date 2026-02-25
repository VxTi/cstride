#include "errors.h"
#include "ast/parsing_context.h"
#include "ast/nodes/control_flow_statements.h"

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
    if (this->get_context()->get_control_flow_blocks().empty())
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Break statement outside of loop",
            this->get_source_fragment()
        );
    }

    const auto break_block = this->get_context()->get_control_flow_blocks().back().second;
    builder->CreateBr(break_block);

    return nullptr;
}

void AstBreakStatement::validate()
{
    if (this->get_context()->get_current_scope_type() != definition::ScopeType::CONTROL_FLOW)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            "Break statement cannot appear outside of a loop or switch statement",
            this->get_source_fragment()
        );
    }
}
