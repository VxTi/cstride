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

    if (reference_token.get_type() != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto segments = parse_segmented_identifier(set, "Expected identifier for named type");
    auto generic_types = parse_generic_arguments(context, set);

    const auto name = resolve_internal_name(segments);

    return parse_type_metadata(
        std::make_unique<AstNamedType>(
            reference_token.get_source_fragment(),
            context,
            name,
            context_type_flags,
            std::move(generic_types)
        ),
        set,
        context_type_flags
    );
}

std::optional<definition::TypeDefinition*> AstNamedType::get_type_definition() const
{
    if (const auto ref_def = this->get_context()->get_type_definition(this->get_name());
        ref_def.has_value())
    {
        return ref_def.value();
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<IAstType>> AstNamedType::get_reference_type() const
{
    if (const auto ref_def = this->get_type_definition();
        ref_def.has_value())
    {
        return ref_def.value()->get_type()->clone_ty();
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<IAstType>> AstNamedType::get_underlying_type() const
{
    const auto& reference_type_definition = this->get_type_definition();

    if (!reference_type_definition.has_value())
    {
        return std::nullopt;
    }

    std::optional<std::unique_ptr<IAstType>> base_type = this->is_generic_overload()
        ? instantiate_generic_type(this, reference_type_definition.value())
        : this->get_reference_type();

    int recursion_guard = 0;

    if (!base_type.has_value())
        return std::nullopt;

    while (const auto* named_reference = cast_type<AstNamedType*>(base_type.value().get()))
    {
        if (named_reference->is_generic_overload())
        {
            if (const auto next_def = named_reference->get_type_definition();
                next_def.has_value())
            {
                base_type = instantiate_generic_type(named_reference, next_def.value());
            }
            else
            {
                break;
            }
        }
        else
        {
            base_type = named_reference->get_reference_type();
        }

        if (++recursion_guard > MAX_RECURSION_DEPTH)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format(
                    "Exceeded maximum recursion depth while resolving base type of '{}'",
                    this->get_name()),
                this->get_source_fragment()
            );
        }
    }

    return std::move(base_type);
}

bool AstNamedType::is_castable_to_impl(IAstType* other)
{
    const auto self_reference_type = get_underlying_type();

    if (!self_reference_type.has_value())
        return false;

    // Check our base type is a primitive, and whether that type is castable to `other`
    if (auto* other_primitive = cast_type<AstPrimitiveType*>(other))
    {
        return self_reference_type.value()->is_castable_to(other_primitive);
    }

    // A named type should be castable to its base type and vice versa
    if (self_reference_type.value()->is_castable_to(other))
    {
        return true;
    }

    // Final case would be to check whether both base types are the same
    if (const auto* other_named_ty = cast_type<AstNamedType*>(other))
    {
        if (const auto second_reference_type = other_named_ty->get_underlying_type();
            second_reference_type.has_value())
        {
            return self_reference_type.value()->equals(*second_reference_type);
        }
    }
    return false;
}

bool AstNamedType::is_assignable_to_impl(IAstType* other)
{
    if (const auto other_named = cast_type<AstNamedType*>(other))
    {
        if (this->get_name() == other_named->get_name())
        {
            return true;
        }
    }

    // It might be the case that we're trying to assign primitive references to a named value, e.g.,
    // type SomePrimitive = i32[]
    // const someVar: SomePrimitive = [1, 2, 3];
    // In this case, `[1, 2, 3]` should be assignable to the base types of `SomePrimitive`
    const auto self_base_type = get_underlying_type();
    if (self_base_type.has_value() && self_base_type.value()->is_assignable_to(other))
    {
        return true;
    }

    if (const auto* other_named_ptr = cast_type<AstNamedType*>(other))
    {
        const auto other_base_type = other_named_ptr->get_underlying_type();
        if (other_base_type.has_value() && this->is_assignable_to(other_base_type.value().get()))
        {
            return true;
        }
    }

    return false;
}

bool AstNamedType::equals(const IAstType& other) const
{
    // Simple naming checks, e.g., "Vec3 == Vec3"
    if (auto* other_named = cast_type<const AstNamedType*>(&other))
    {
        if (this->get_name() == other_named->get_name())
        {
            // If both aren't generic, then name comparison should suffice
            if (!this->is_generic_overload() && !other_named->is_generic_overload())
            {
                return true;
            }

            // If both are generic, they must have the same number of parameters
            if (this->get_instantiated_generic_types().size() == other_named->get_instantiated_generic_types().size())
            {
                // And all parameters must be equal
                bool all_equal = true;
                for (size_t i = 0; i < this->get_instantiated_generic_types().size(); i++)
                {
                    if (!this->get_instantiated_generic_types()[i]->equals(
                        other_named->get_instantiated_generic_types()[i]))
                    {
                        all_equal = false;
                        break;
                    }
                }
                if (all_equal)
                {
                    return true;
                }
            }
        }
    }

    // If it's not a named type, or names/generics didn't match, check underlying types
    const auto self_underlying = this->get_underlying_type();
    if (self_underlying.has_value())
    {
        return self_underlying.value()->equals(other);
    }

    return false;
}

std::unique_ptr<IAstNode> AstNamedType::clone()
{
    GenericTypeList generic_types;
    generic_types.reserve(this->_generic_types.size());
    for (const auto& generic_type : this->_generic_types)
    {
        generic_types.push_back(generic_type->clone_ty());
    }
    return std::make_unique<AstNamedType>(
        this->get_source_fragment(),
        this->get_context(),
        this->_name,
        this->get_flags(),
        std::move(generic_types)
    );
}
