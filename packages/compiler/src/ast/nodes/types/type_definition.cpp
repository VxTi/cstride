#include "ast/nodes/type_definition.h"

#include "ast/parsing_context.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::unique_ptr<AstTypeDefinition> stride::ast::parse_type_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_TYPE);
    const auto& ref_pos = reference_token.get_source_fragment();

    const auto type_name = set.expect(TokenType::IDENTIFIER, "Expected type name").get_lexeme();

    set.expect(TokenType::EQUALS);

    auto type = parse_type(context, set, "Expected type definition");
    const auto& last_token = set.expect(TokenType::SEMICOLON, "Expected ';' after type definition");
    const auto& last_pos = last_token.get_source_fragment();

    const auto source_fragment = SourceFragment(
        set.get_source(),
        ref_pos.offset,
        last_pos.offset + last_pos.length - ref_pos.offset
    );
    const auto type_name_symbol = resolve_internal_name(
        context->get_name(),
        source_fragment,
        { type_name });

    context->define_type(type_name_symbol, type->clone_ty());

    return std::make_unique<AstTypeDefinition>(
        source_fragment,
        context,
        type_name,
        std::move(type),
        modifier
    );
}

void AstTypeDefinition::resolve_forward_references(
    ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    this->_type->resolve_forward_references(context, module, builder);
}

llvm::Value* AstTypeDefinition::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return this->_type->codegen(module, builder);
}

std::unique_ptr<IAstNode> AstTypeDefinition::clone()
{
    return std::make_unique<AstTypeDefinition>(
        this->get_source_fragment(),
        this->get_context(),
        this->_name,
        this->_type->clone_ty(),
        this->_visibility
    );
}
