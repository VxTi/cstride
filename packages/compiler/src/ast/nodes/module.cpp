#include "ast/nodes/module.h"

#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/nodes/blocks.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <format>

using namespace stride::ast;
using namespace stride::ast::definition;

// TODO: Ensure symbols are prefixed with module for proper lookup

std::string AstModule::to_string()
{
    return std::format(
        "Module ({}): {}",
        this->get_name(),
        this->get_body()->to_string()
    );
}

std::unique_ptr<AstModule> stride::ast::parse_module_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_MODULE);

    const auto module_identifier =
        set
       .expect(TokenType::IDENTIFIER, "Expected module name after 'module' keyword")
       .get_lexeme();

    // If the module is defined in another module, we might already have a context name.
    // This means we'll have to extend the current name so that other callees can access nested
    // modules. Otherwise, the symbols will be wrongly prefixed when internalizing their names.
    const std::vector<std::string> module_name_segments = !context->get_name().empty()
        ? std::vector{ context->get_name(), module_identifier }
        : std::vector{ module_identifier };

    const auto module_name = resolve_internal_name(module_name_segments);

    const auto module_context = std::make_shared<ParsingContext>(
        module_name,
        ContextType::MODULE,
        context);
    auto module_body = parse_block(module_context, set);

    return std::make_unique<AstModule>(
        reference_token.get_source_fragment(),
        module_context,
        module_name,
        std::move(module_body)
    );
}

llvm::Value* AstModule::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    if (!this->_body)
    {
        return nullptr;
    }

    return this->_body->codegen(module, builder);
}

void AstModule::resolve_forward_references(
    ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    if (!this->_body)
    {
        return;
    }

    this->_body->resolve_forward_references(context, module, builder);
}

std::unique_ptr<IAstNode> AstModule::clone()
{
    return std::make_unique<AstModule>(
        this->get_source_fragment(),
        this->get_context(),
        this->_name,
        this->_body->clone_as<AstBlock>()
    );
}
