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
    const std::string& error,
    const int type_flags
)
{
    if (auto primitive = parse_primitive_type_optional(context, set, type_flags);
        primitive.has_value())
    {
        return std::move(primitive.value());
    }

    if (auto named_type = parse_named_type_optional(context, set, type_flags);
        named_type.has_value())
    {
        return std::move(named_type.value());
    }


    if (auto function_type = parse_function_type_optional(context, set, type_flags);
        function_type.has_value())
    {
        return std::move(function_type.value());
    }

    if (auto tuple_type = parse_tuple_type_optional(context, set, type_flags);
        tuple_type.has_value())
    {
        return std::move(tuple_type.value());
    }

    if (auto struct_type = parse_struct_type_optional(context, set, type_flags);
        struct_type.has_value())
    {
        return std::move(struct_type.value());
    }

    set.throw_error(error);
}

llvm::Type* stride::ast::type_to_llvm_type(
    IAstType* type,
    llvm::Module* module,
    size_t recursion_guard
)
{
    if (!type)
    {
        return nullptr;
    }
    const auto context = type->get_context();

    // Wrapping T -> Optional<T>
    if (type->is_optional())
    {
        const auto inner = type->clone();
        // Remove the optional flag so that it doesn't recursively enter this same scope
        inner->set_flags(inner->get_flags() & ~SRFLAG_TYPE_OPTIONAL);
        llvm::Type* inner_type =
            type_to_llvm_type(inner.get(), module);

        return llvm::StructType::get(
            module->getContext(),
            {
                llvm::Type::getInt1Ty(module->getContext()), // has_value
                inner_type                                   // value (T)
            });
    }

    if (type->is_pointer())
    {
        return llvm::PointerType::get(module->getContext(), 0);
    }

    if (const auto* array_ty = cast_type<AstArrayType*>(type))
    {
        llvm::Type* element_type = type_to_llvm_type(
            array_ty->get_element_type(),
            module
        );

        if (!element_type)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "Unable to resolve internal type for array element",
                array_ty->get_source_fragment()
            );
        }

        return llvm::ArrayType::get(element_type, array_ty->get_initial_length());
    }

    if (const auto* primitive_ty = cast_type<AstPrimitiveType*>(type))
    {
        switch (primitive_ty->get_type())
        {
        case PrimitiveType::INT8:
        case PrimitiveType::UINT8:
            return llvm::Type::getInt8Ty(module->getContext());
        case PrimitiveType::INT16:
        case PrimitiveType::UINT16:
            return llvm::Type::getInt16Ty(module->getContext());
        case PrimitiveType::INT32:
        case PrimitiveType::UINT32:
            return llvm::Type::getInt32Ty(module->getContext());
        case PrimitiveType::INT64:
        case PrimitiveType::UINT64:
            return llvm::Type::getInt64Ty(module->getContext());
        case PrimitiveType::FLOAT32:
            return llvm::Type::getFloatTy(module->getContext());
        case PrimitiveType::FLOAT64:
            return llvm::Type::getDoubleTy(module->getContext());
        case PrimitiveType::BOOL:
            return llvm::Type::getInt1Ty(module->getContext());
        case PrimitiveType::CHAR:
            return llvm::Type::getInt8Ty(module->getContext());
        case PrimitiveType::STRING:
            return llvm::PointerType::get(module->getContext(), 0);
        case PrimitiveType::VOID:
        default:
            return llvm::Type::getVoidTy(module->getContext());
        }
    }

    if (const auto* named_ty = cast_type<AstNamedType*>(type))
    {
        // If it's a pointer, we don't even need to look up the struct name
        // to return the LLVM type, because all pointers are the same.
        // However, usually you want to validate the type exists first.
        if (named_ty->is_pointer())
        {
            return llvm::PointerType::get(module->getContext(), 0);
        }

        const auto ref_type = named_ty->get_reference_type();
        if (!ref_type.has_value())
        {
            throw parsing_error(
                ErrorType::REFERENCE_ERROR,
                std::format("Reference type '{}' not found", named_ty->get_name()),
                named_ty->get_source_fragment()
            );
        }

        if (++recursion_guard > MAX_RECURSION_DEPTH)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "Maximum recursion depth exceeded when resolving type",
                named_ty->get_source_fragment()
            );
        }

        return type_to_llvm_type(ref_type.value().get(), module, recursion_guard);
    }

    if (const auto* struct_type = cast_type<AstStructType*>(type))
    {
        const auto& struct_name = struct_type->get_internalized_name();
        llvm::StructType* struct_ty = llvm::StructType::getTypeByName(
            module->getContext(),
            struct_name
        );
        if (!struct_ty)
        {
            throw parsing_error(
                ErrorType::REFERENCE_ERROR,
                "Struct type not found",
                struct_type->get_source_fragment()
            );
        }

        return struct_ty;
    }

    if (cast_type<AstFunctionType*>(type))
    {
        return llvm::PointerType::get(module->getContext(), 0);
    }

    return nullptr;
}

std::unique_ptr<IAstType> stride::ast::get_dominant_field_type(
    IAstType* lhs,
    IAstType* rhs
)
{
    const auto& context = lhs->get_context();
    const auto* lhs_primitive = dynamic_cast<AstPrimitiveType*>(lhs);
    const auto* rhs_primitive = dynamic_cast<AstPrimitiveType*>(rhs);
    const auto* lhs_named = dynamic_cast<AstNamedType*>(lhs);
    const auto* rhs_named = dynamic_cast<AstNamedType*>(rhs);

    // Error if one is named and the other is primitive
    if ((lhs_named && rhs_primitive) || (lhs_primitive && rhs_named))
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot mix primitive type with named type",
            lhs->get_source_fragment());
    }

    // Both must be primitives for dominance calculation
    if (!lhs_primitive || !rhs_primitive)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot compute dominant type for non-primitive types",
            lhs->get_source_fragment());
    }

    const bool are_both_sides_integers =
        lhs_primitive->is_integer_ty() && rhs_primitive->is_integer_ty();
    const bool are_both_sides_floats = lhs_primitive->is_fp() && rhs_primitive->
        is_fp();

    // TODO: Handle unsigned / signed properly

    // If both sides are the same, we'll just return the one with the highest byte count
    // E.g., dominant type (fp32, fp64) will yield fp64, (int32, int64) will yield int64
    if (are_both_sides_floats || are_both_sides_integers)
    {
        return lhs_primitive->bit_count() >= rhs_primitive->bit_count()
            ? lhs_primitive->clone()
            : rhs_primitive->clone();
    }

    // If LHS is a float, but the RHS is not, we'll have to convert the resulting
    // type into a float with the highest byte size
    // The same holds true for visa vesa.
    if ((lhs_primitive->is_fp() && !rhs_primitive->is_fp()) ||
        (rhs_primitive->is_fp() && !lhs_primitive->is_fp()))
    {
        // If the RHS has a higher byte size, we need to promote the RHS to a floating point type
        // and return the highest byte size
        if (rhs_primitive->bit_count() > lhs_primitive->bit_count())
        {
            return std::make_unique<AstPrimitiveType>(
                lhs_primitive->get_source_fragment(),
                context,
                PrimitiveType::FLOAT64,
                rhs_primitive->bit_count(),
                rhs_primitive->get_flags());
        }

        // Otherwise, just return the LHS as the dominant type (float32 / float64)
        return lhs_primitive->clone();
    }

    const std::vector references = {
        ErrorSourceReference(lhs->to_string(), lhs->get_source_fragment()),
        ErrorSourceReference(rhs->get_type_name(), rhs->get_source_fragment())
    };

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        "Cannot compute dominant type for incompatible primitive types",
        references);
}

std::optional<AstStructType*> stride::ast::get_struct_type_from_type(IAstType* type)
{
    auto base_struct_type = cast_type<AstStructType*>(type);

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
