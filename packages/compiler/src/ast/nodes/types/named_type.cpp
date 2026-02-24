#include "ast/nodes/types.h"

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_named_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    int context_type_flags
)
{
    // Custom types are identifiers in the type position.
    const auto reference_token = set.peek_next();

    if (set.peek_next_eq(TokenType::STAR))
    {
        set.next();
        context_type_flags |= SRFLAG_TYPE_PTR;
    }
    if (set.peek_next().get_type() != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto name = set.next().get_lexeme();

    auto named_type = std::make_unique<AstNamedType>(
        reference_token.get_source_fragment(),
        context,
        name,
        context_type_flags);

    return parse_type_metadata(std::move(named_type), set, context_type_flags);
}

bool AstNamedType::equals(IAstType& other)
{
    if (auto* other_named = cast_type<AstNamedType*>(&other))
    {
        return this->get_formatted_name() == other_named->get_formatted_name();
    }

    // The other type might be a primitive "NIL" type,
    // then we consider the types equal if this is optional
    if (const auto* other_primitive = cast_type<AstPrimitiveType*>(&other))
    {
        return other_primitive->get_type() == PrimitiveType::NIL
            && this->is_optional();
    }
    return false;
}