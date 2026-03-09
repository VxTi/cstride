#include "ast/generics.h"

#include "ast/nodes/types.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

GenericParameterList stride::ast::parse_generic_declaration(TokenSet& set)
{
    GenericParameterList generic_params;
    if (set.peek_next_eq(TokenType::LT))
    {
        set.next();
        generic_params.push_back(
            set.expect(TokenType::IDENTIFIER, "Expected generic parameter name").get_lexeme()
        );

        while (set.peek_next_eq(TokenType::COMMA))
        {
            set.next();
            generic_params.push_back(
                set.expect(TokenType::IDENTIFIER, "Expected generic parameter name").get_lexeme()
            );
        }

        set.expect(TokenType::GT);
    }
    return generic_params;
}

GenericTypeList stride::ast::parse_generic_arguments(const std::shared_ptr<ParsingContext>& context, TokenSet& set)
{
    GenericTypeList generic_params;
    if (set.peek_next_eq(TokenType::LT))
    {
        set.next();
        generic_params.push_back(
            parse_type(context, set, "Expected generic parameter name")
        );

        while (set.peek_next_eq(TokenType::COMMA))
        {
            set.next();
            generic_params.push_back(
                parse_type(context, set, "Expected generic parameter name")
            );
        }

        set.expect(TokenType::GT);
    }
    return std::move(generic_params);
}
