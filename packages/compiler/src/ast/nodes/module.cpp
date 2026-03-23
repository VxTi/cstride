#include "ast/nodes/module.h"

#include "ast/symbol_table.h"
#include "ast/nodes/blocks.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <format>

using namespace stride::ast;
using namespace stride::ast::definition;

std::string AstModule::to_string()
{
    return std::format(
        "Module ({}): {}",
        this->get_name(),
        this->get_body()->to_string()
    );
}

std::unique_ptr<AstModule> stride::ast::parse_module_statement(
    TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_MODULE);

    const auto& module_identifier_tok =
        set.expect(TokenType::IDENTIFIER, "Expected module name after 'module' keyword");
    const auto module_name = module_identifier_tok.get_lexeme();

    auto module_body = parse_block(set);

    return std::make_unique<AstModule>(
        SourcePosition::join(reference_token.get_source_position(), module_identifier_tok.get_source_position()),
        module_name,
        std::move(module_body)
    );
}

llvm::Value* AstModule::codegen(
    SymbolTable* symbol_table,
    llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return this->_body->codegen(symbol_table, module, builder);
}

void AstModule::validate(const SymbolTable* symbol_table)
{
    this->_body->validate(symbol_table);
}

void AstModule::resolve_forward_references(
    SymbolTable* symbol_table,
    llvm::Module* module, llvm::IRBuilderBase* builder)
{
    this->_body->resolve_forward_references(symbol_table, module, builder);
}

std::unique_ptr<IAstNode> AstModule::clone()
{
    return std::make_unique<AstModule>(
        this->get_source_position(),
        this->_name,
        this->_body->clone_as<AstBlock>()
    );
}
