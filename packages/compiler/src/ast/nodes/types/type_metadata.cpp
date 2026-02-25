#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

bool is_array_notation(const TokenSet& set)
{
    return set.peek_eq(TokenType::LSQUARE_BRACKET, 0) && set.peek_eq(
        TokenType::RSQUARE_BRACKET,
        1);
}

std::unique_ptr<IAstType> stride::ast::parse_type_metadata(
    std::unique_ptr<IAstType> base_type,
    TokenSet& set,
    int context_type_flags
)
{
    const auto src_pos = set.peek_next().get_source_fragment();
    int offset = 0;

    while (is_array_notation(set))
    {
        offset += 2;
        set.skip(2);
        base_type = std::make_unique<AstArrayType>(
            SourceFragment(
                base_type->get_source(),
                src_pos.offset,
                src_pos.length + offset),
            base_type->get_context(),
            std::move(base_type),
            0);
    }

    // If the preceding token is a question mark, the type is determined
    // to be optional.
    // An example of this would be `int32?` or `int32[]?`
    if (set.peek_next_eq(TokenType::QUESTION))
    {
        set.skip(1);
        context_type_flags |= SRFLAG_TYPE_OPTIONAL;
    }

    base_type->set_flags(base_type->get_flags() | context_type_flags);

    return std::move(base_type);
}

bool AstArrayType::equals(IAstType& other)
{
    if (const auto* other_array = cast_type<AstArrayType*>(&other))
    {
        // Length here doesn't matter; merely the types should be the same.
        return this->_element_type->equals(*other_array->_element_type);
    }
    return false;
}
