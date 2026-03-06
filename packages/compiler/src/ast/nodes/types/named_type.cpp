#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_named_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    int context_type_flags
)
{
    // Custom types are identifiers in the type position.
    const auto reference_token = set.peek_next();

    if (set.peek_next().get_type() != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto segments = parse_segmented_identifier(set, "Expected identifier for named type");

    const auto name = resolve_internal_name(segments);

    auto named_type = std::make_unique<AstNamedType>(
        reference_token.get_source_fragment(),
        context,
        name,
        context_type_flags);

    return parse_type_metadata(std::move(named_type), set, context_type_flags);
}

std::optional<std::unique_ptr<IAstType>> AstNamedType::get_reference_type() const
{
    if (const auto ref_def = this->get_context()->get_type_definition(this->get_name());
        ref_def.has_value())
    {
        return ref_def.value()->get_type()->clone_ty();
    }
    return std::nullopt;
}

bool AstNamedType::is_assignable_to_impl(IAstType* other) const
{
    if (const auto other_named = cast_type<AstNamedType*>(other))
    {
        return this->get_name() == other_named->get_name();
    }
    return false;
}

bool AstNamedType::equals(IAstType& other)
{
    // Simple naming checks, e.g., "Vec3 == Vec3"
    if (auto* other_named = cast_type<AstNamedType*>(&other))
    {
        return this->get_type_name() == other_named->get_type_name();
    }

    // The other type might be a primitive "NIL" type,
    // then we consider the types equal if this is optional
    if (const auto* other_primitive = cast_type<AstPrimitiveType*>(&other))
    {
        return other_primitive->get_primitive_type() == PrimitiveType::NIL
            && this->is_optional();
    }
    return false;
}
