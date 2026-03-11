#include "ast/nodes/types.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

using namespace stride::ast;

std::unique_ptr<IAstType> stride::ast::parse_type(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    const TypeParsingOptions& options
)
{
    if (auto primitive = parse_primitive_type_optional(context, set, options);
        primitive.has_value())
    {
        return std::move(primitive.value());
    }

    if (auto named_type = parse_named_type_optional(context, set, options);
        named_type.has_value())
    {
        return std::move(named_type.value());
    }

    if (auto function_type = parse_function_type_optional(context, set, options);
        function_type.has_value())
    {
        return std::move(function_type.value());
    }

    if (auto tuple_type = parse_tuple_type_optional(context, set, options);
        tuple_type.has_value())
    {
        return std::move(tuple_type.value());
    }

    if (auto struct_type = parse_object_type_optional(context, set, options);
        struct_type.has_value())
    {
        return std::move(struct_type.value());
    }

    set.throw_error(options.error_message);
}

llvm::Type* IAstType::get_llvm_type(llvm::Module* module)
{

    if (this->is_optional())
    {
        const auto non_optional = this->clone_ty();

        // Remove the optional flag so that it doesn't recursively enter this same scope
        non_optional->set_flags(non_optional->get_flags() & ~SRFLAG_TYPE_OPTIONAL);

        llvm::Type* value_ty = non_optional->get_llvm_type(module);

        return llvm::StructType::get(
            module->getContext(),
            {
                llvm::Type::getInt1Ty(module->getContext()), // has_value
                value_ty                                     // value (T)
            });
    }

    if (this->is_pointer())
    {
        return llvm::PointerType::get(module->getContext(), 0);
    }

    return this->get_llvm_type_impl(module);
}

bool IAstType::is_assignable_to(IAstType* other)
{
    // A type is not assignable to another if the source is optional but the target is not.
    // E.g., `i32?` is not assignable to `i32`.
    if (this->is_optional() && !other->is_optional())
    {
        return false;
    }

    if (this->equals(other))
    {
        return true;
    }

    // Check if LHS is optional and RHS is nil
    if (this->is_optional() && other->is_primitive() &&
        cast_type<AstPrimitiveType*>(other)->get_primitive_type() == PrimitiveType::NIL)
    {
        return true;
    }

    // Check if LHS is nil and RHS is optional
    if (other->is_optional() && this->is_primitive() &&
        cast_type<AstPrimitiveType*>(this)->get_primitive_type() == PrimitiveType::NIL)
    {
        return true;
    }

    // Try to resolve named types on both sides to check assignability
    if (auto* this_named = cast_type<AstAliasType*>(this))
    {
        if (const auto self_base = this_named->get_underlying_type();
            self_base.has_value() && self_base.value()->is_assignable_to(other))
        {
            return true;
        }
    }

    if (auto* other_named = cast_type<AstAliasType*>(other))
    {
        if (const auto other_base = other_named->get_underlying_type();
            other_base.has_value() && this->is_assignable_to(other_base.value().get()))
        {
            return true;
        }
    }

    // Otherwise it's up to the other implementors to decide.
    return this->is_assignable_to_impl(other);
}

bool IAstType::is_castable_to(IAstType* other)
{
    // A type is not castable to another if the source is optional but the target is not.
    // E.g., `i32?` is not castable to `i32`.
    if (this->is_optional() && !other->is_optional())
    {
        return false;
    }

    if (this->equals(other))
    {
        return true;
    }

    return this->is_castable_to_impl(other);
}

AstPrimitiveType* extract_primitive_reference_types(IAstType* type)
{
    if (!type)
    {
        return nullptr;
    }

    if (const auto named = cast_type<AstAliasType*>(type))
    {
        const auto ref_type = named->get_underlying_type();

        if (!ref_type.has_value())
        {
            return nullptr;
        }

        return extract_primitive_reference_types(ref_type.value().get());
    }

    if (const auto* array_type = cast_type<AstArrayType*>(type))
    {
        return extract_primitive_reference_types(array_type->get_element_type());
    }

    return cast_type<AstPrimitiveType*>(type);
}

/// Dominant field comparison can only be done on primitive types, e.g., i32 vs f64, i32 vs i64, etc.
/// If we have named types on either side, we have to extract their primitive types, if they reference so.
std::unique_ptr<IAstType> stride::ast::get_dominant_field_type(
    IAstType* lhs,
    IAstType* rhs
)
{
    const auto& context = lhs->get_context();

    // Resolves LHS and RHS into possibly primitive types, if they reference so
    const auto& lhs_primitive_ty = extract_primitive_reference_types(lhs);
    const auto& rhs_primitive_ty = extract_primitive_reference_types(rhs);

    // Check whether we're mixing types, e.g., `10 + <struct>`
    if ((!lhs_primitive_ty && rhs_primitive_ty) || (lhs_primitive_ty && !rhs_primitive_ty))
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot mix primitive type with named type",
            {
                ErrorSourceReference(lhs->get_type_name(), lhs->get_source_fragment()),
                ErrorSourceReference(rhs->get_type_name(), rhs->get_source_fragment())
            }
        );
    }

    // If LHS and RHS are not primitive, we can still compute a dominant type if they are equal
    if (!lhs_primitive_ty || !rhs_primitive_ty)
    {
        if (lhs->equals(rhs))
        {
            return lhs->clone_ty();
        }

        // If one is a named type and the other is its base type, we can also return the dominant type
        if (const auto lhs_named = cast_type<AstAliasType*>(lhs))
        {
            if (const auto base = lhs_named->get_underlying_type(); base.has_value() && base.value()->equals(rhs))
            {
                return rhs->clone_ty();
            }
        }

        if (const auto rhs_named = cast_type<AstAliasType*>(rhs))
        {
            if (const auto base = rhs_named->get_underlying_type(); base.has_value() && base.value()->equals(lhs))
            {
                return lhs->clone_ty();
            }
        }

        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot compute dominant type for non-primitive types",
            !lhs_primitive_ty ? lhs->get_source_fragment() : rhs->get_source_fragment()
        );
    }

    // If LHS is a nil primitive, we prefer the RHS type, since the LHS can be safely ignored in this context (e.g., optional types)
    if (lhs_primitive_ty->get_primitive_type() == PrimitiveType::NIL)
    {
        return rhs->clone_ty();
    }

    // Same holds true here
    if (rhs_primitive_ty->get_primitive_type() == PrimitiveType::NIL)
    {
        return lhs->clone_ty();
    }

    const bool are_both_sides_integers =
        lhs_primitive_ty->is_integer_ty() && rhs_primitive_ty->is_integer_ty();
    const bool are_both_sides_floats = lhs_primitive_ty->is_fp() && rhs_primitive_ty->
        is_fp();

    // TODO: Handle unsigned / signed properly

    // If both sides are the same, we'll just return the one with the highest byte count
    // E.g., dominant type (fp32, fp64) will yield fp64, (i32, i64) will yield i64
    if (are_both_sides_floats || are_both_sides_integers)
    {
        return lhs_primitive_ty->bit_count() >= rhs_primitive_ty->bit_count()
            ? lhs->clone_ty()
            : rhs->clone_ty();
    }

    // If LHS is a float, but the RHS is not, we'll have to convert the resulting
    // type into a float with the highest byte size
    // The same holds true for visa vesa.
    if ((lhs_primitive_ty->is_fp() && !rhs_primitive_ty->is_fp()) ||
        (rhs_primitive_ty->is_fp() && !lhs_primitive_ty->is_fp()))
    {
        // If the RHS has a higher byte size, we need to promote the RHS to a floating point type
        // and return the highest byte size
        if (rhs_primitive_ty->bit_count() > lhs_primitive_ty->bit_count())
        {
            // RHS is dominant
            return std::make_unique<AstPrimitiveType>(
                rhs->get_source_fragment(),
                context,
                PrimitiveType::FLOAT64,
                rhs->get_flags()
            );
        }

        // Otherwise, just return the LHS as the dominant type (f32 / f64)
        // LHS is dominant
        return lhs->clone_ty();
    }

    const std::vector references = {
        ErrorSourceReference(lhs->to_string(), lhs->get_source_fragment()),
        ErrorSourceReference(rhs->get_type_name(), rhs->get_source_fragment())
    };

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        "Cannot compute dominant type for incompatible primitive types",
        references
    );
}

std::optional<AstObjectType*> stride::ast::get_object_type_from_type(IAstType* type)
{
    auto base_struct_type = cast_type<AstObjectType*>(type);

    if (base_struct_type)
    {
        return base_struct_type;
    }

    // It might be a named type
    base_struct_type = type->get_context()->get_struct_type(type->get_type_name())
                            .value_or(nullptr);

    if (!base_struct_type)
    {
        return std::nullopt;
    }

    return base_struct_type;
}
