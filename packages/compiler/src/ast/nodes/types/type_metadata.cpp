#include "ast/casting.h"
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
            0
        );
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

std::unique_ptr<IAstNode> AstArrayType::clone()
{
    return std::make_unique<AstArrayType>(
        this->get_source_fragment(),
        this->get_context(),
        this->_element_type->clone_ty(),
        this->_initial_length
    );
}

std::string AstArrayType::to_string()
{
    if (cast_type<AstFunctionType*>(this->get_element_type()))
    {
        return std::format(
            "({}{})[]",
            this->_element_type->to_string(),
            (this->get_flags() & SRFLAG_TYPE_OPTIONAL) != 0 ? "?" : "");
    }
    return std::format(
        "{}{}[]",
        this->_element_type->to_string(),
        (this->get_flags() & SRFLAG_TYPE_OPTIONAL) != 0 ? "?" : "");
}

bool AstArrayType::equals(IAstType& other)
{
    if (const auto* other_array = cast_type<AstArrayType*>(&other))
    {
        return this->_element_type->equals(other_array->_element_type);
    }
    return false;
}

bool AstArrayType::is_assignable_to_impl(IAstType* other)
{
    // If we're trying to assign a named type to an array, we have to check
    // whether the referencing type is assignable to this array's element type,
    // e.g., for `type SomeArray = [1, 2, 3]`, `equals(int32[], SomeArray)` should check
    // whether `[1, 2, 3]` in `SomeArray` (int32[]) is assignable to `Array(int32)`
    if (const auto * other_named = cast_type<AstNamedType*>(other))
    {
        const auto reference_type = other_named->get_base_reference_type();
        if (!reference_type.has_value())
        {
            return false;
        }

        // Validate whether the reference type of `other_named` is assignable to
        return this->is_assignable_to(reference_type.value().get());
    }
    // If both are arrays, we can just simply check whether their element types are equal
    // This is handled in the `equals` case.
    return this->equals(*other);
}

bool AstArrayType::is_castable_to_impl(IAstType* other)
{
    // If we're trying to cast an array to a named type, we have to check
    // whether the referencing type is assignable to this array's element type,
    // e.g., for `type SomeArray = [1, 2, 3]`, `equals(int32[], SomeArray)` should check
    // whether `[1, 2, 3]` in `SomeArray` (int32[]) is assignable to `Array(int32)`
    if (const auto* other_named = cast_type<AstNamedType*>(other))
    {
        const auto reference_type = other_named->get_base_reference_type();
        if (!reference_type.has_value())
        {
            return false;
        }

        // Validate whether the reference type of `other_named` is assignable to
        return this->is_castable_to(reference_type.value().get());
    }

    return false;
}
