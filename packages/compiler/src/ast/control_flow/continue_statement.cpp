#include "ast/parsing_context.h"
#include "ast/nodes/control_flow_statements.h"

using namespace stride::ast;

std::unique_ptr<AstContinueStatement> stride::ast::parse_continue_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto& reference_token = set.expect(TokenType::KEYWORD_CONTINUE);
    set.expect(TokenType::SEMICOLON, "Expected ';' after 'continue' statement");

    return std::make_unique<AstContinueStatement>(reference_token.get_source_fragment(), context);
}

llvm::Value* AstContinueStatement::codegen(llvm::Module* module, llvm::IRBuilder<>* builder)
{
    if (this->get_context()->get_control_flow_blocks().empty())
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Continue statement outside of loop",
            this->get_source_fragment()
        );
    }

    const auto continue_block = this->get_context()->get_control_flow_blocks().back().first;
    builder->CreateBr(continue_block);

    // Since we branched, create a new block for unreachable code, but since it's a statement, return nullptr
    return nullptr;
}

void AstContinueStatement::validate()
{
    if (this->get_context()->get_current_scope_type() != definition::ScopeType::CONTROL_FLOW)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            "Continue statement cannot appear outside of a loop or switch statement",
            this->get_source_fragment()
        );
    }
}
