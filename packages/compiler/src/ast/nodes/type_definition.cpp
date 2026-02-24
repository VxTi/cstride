#pragma once

#include "ast/nodes/type_definition.h"

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

    context->define_type(type_name, type->clone());

    const auto& last_token = set.expect(TokenType::SEMICOLON, "Expected ';' after type definition");
    const auto& last_pos = last_token.get_source_fragment();

    return std::make_unique<AstTypeDefinition>(
        SourceFragment(
            set.get_source(),
            ref_pos.offset,
            last_pos.offset + last_pos.length - ref_pos.offset
        ),
        context,
        type_name,
        std::move(type)
    );
}
