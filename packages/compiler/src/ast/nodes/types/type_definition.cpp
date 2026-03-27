#include "ast/nodes/type_definition.h"

#include "ast/symbol_table.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::unique_ptr<AstTypeDefinition> stride::ast::parse_type_definition(
    TokenSet& set,
    VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_TYPE);
    const auto& ref_pos = reference_token.get_source_position();

    const auto type_name = set.expect(TokenType::IDENTIFIER, "Expected type name").get_lexeme();

    GenericParameterList generic_params = parse_generic_declaration(set);

    set.expect(TokenType::EQUALS);

    auto type = parse_type(
        set,
        { "Expected type definition", type_name, SRFLAG_NONE, generic_params }
    );
    const auto& last_token = set.expect(TokenType::SEMICOLON, "Expected ';' after type definition");
    const auto& last_pos = last_token.get_source_position();

    return std::make_unique<AstTypeDefinition>(
        SourcePosition::join(ref_pos, last_pos),
        type_name,
        std::move(type),
        modifier,
        generic_params
    );
}

std::unique_ptr<IAstNode> AstTypeDefinition::clone()
{
    return std::make_unique<AstTypeDefinition>(
        this->get_source_position(),
        this->_name,
        this->_type->clone(),
        this->_visibility,
        this->_generic_parameters
    );
}
