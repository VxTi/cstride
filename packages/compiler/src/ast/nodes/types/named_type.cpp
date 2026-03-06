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

std::optional<std::unique_ptr<IAstType>> AstNamedType::get_base_reference_type() const
{
    std::optional<std::unique_ptr<IAstType>> base_type = this->get_reference_type();
    int recursion_guard = 0; // Prevent self-referencing types causing infinite loops

    while (const auto* named_reference = cast_type<AstNamedType*>(base_type.value().get()))
    {
        base_type = named_reference->get_reference_type();
        if (++recursion_guard > MAX_RECURSION_DEPTH)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format(
                    "Exceeded maximum recursion depth of {} while resolving base type of '{}'",
                    MAX_RECURSION_DEPTH,
                    this->get_name()),
                this->get_source_fragment()
            );
        }
    }

    return base_type;
}

bool AstNamedType::is_assignable_to_impl(IAstType* other)
{
    if (const auto other_named = cast_type<AstNamedType*>(other))
    {
        return this->get_name() == other_named->get_name();
    }

    // It might be the case that we're trying to assign primitive references to a named value, e.g.,
    // type SomePrimitive = int32[]
    // const someVar: SomePrimitive = [1, 2, 3];
    // In this case, `[1, 2, 3]` should be assignable to the base types of `SomePrimitive`
    if (const auto other_primitive = cast_type<AstPrimitiveType*>(other))
    {
        const auto base_type = get_base_reference_type();
        return base_type.has_value() && base_type.value()->equals(*other_primitive);
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
