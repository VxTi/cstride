#include "ast/nodes/module.h"

#include "ast/identifiers.h"
#include "ast/parser.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

// TODO: Ensure symbols are prefixed with module for proper lookup

std::string AstModule::to_string()
{
    return std::format("Module ({})", this->get_name());
}

bool stride::ast::is_module_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_MODULE);
}

std::unique_ptr<AstModule> stride::ast::parse_module_statement(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_MODULE);

    const auto module_name = set.expect(
        TokenType::IDENTIFIER,
        "Expected module name after 'module' keyword"
    ).get_lexeme();

    const auto module_scope = std::make_shared<SymbolRegistry>(scope, ScopeType::MODULE);
    auto module_body = parse_block(module_scope, set);

    return std::make_unique<AstModule>(
        set.get_source(),
        reference_token.get_source_position(),
        scope,
        module_name,
        std::move(module_body)
    );
}

llvm::Value* AstModule::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    if (!this->_body)
    {
        return nullptr;
    }

    return this->_body->codegen(
        scope, module, context, builder
    );
}

void AstModule::resolve_forward_references(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    if (!this->_body)
    {
        return;
    }

    this->_body->resolve_forward_references(scope, module, context, builder);
}
