#include "ast/optionals.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;

/// Checks whether the provided type conforms to the required data layout
bool stride::ast::is_optional_wrapped_type(const llvm::Type* type)
{
    if (const auto* struct_type = llvm::dyn_cast<llvm::StructType>(type))
    {
        if (struct_type->getNumElements() != OPT_ELEMENT_COUNT) return false;

        const auto has_value_ty = struct_type->getElementType(OPT_IDX_HAS_VALUE);
        const auto element_ty = struct_type->getElementType(OPT_IDX_ELEMENT_TYPE); // Type "T"; can be ptr or primitive.

        return has_value_ty != nullptr
            && element_ty != nullptr
            && has_value_ty->isIntegerTy(OPT_HAS_VALUE_BIT_COUNT);
    }
    return false;
}

llvm::Value* stride::ast::wrap_into_optional_value(
    llvm::Value* value,
    llvm::Type* optional_ty,
    llvm::IRBuilder<>* builder
)
{
    // If it's already wrapped, just return it.
    if (is_optional_wrapped_type(value->getType()))
    {
        return value;
    }

    // We must know what optional wrapper type we are building.
    if (!optional_ty || !is_optional_wrapped_type(optional_ty))
    {
        return nullptr;
    }

    llvm::Type* inner_ty = llvm::cast<llvm::StructType>(optional_ty)->getElementType(OPT_IDX_ELEMENT_TYPE);

    // nil -> { i1 false, undef inner }
    if (llvm::isa<llvm::ConstantPointerNull>(value))
    {
        llvm::Value* wrapped = llvm::UndefValue::get(optional_ty);
        wrapped = builder->CreateInsertValue(
            wrapped,
            builder->getInt1(OPT_NO_VALUE),
            {OPT_IDX_HAS_VALUE}
        );
        return wrapped;
    }

    // If needed, cast the value to the inner type.
    if (value->getType() != inner_ty)
    {
        if (value->getType()->isIntegerTy() && inner_ty->isIntegerTy())
        {
            value = builder->CreateIntCast(value, inner_ty, /*isSigned*/ true);
        }
        else if (value->getType()->isPointerTy() && inner_ty->isPointerTy())
        {
            value = builder->CreatePointerCast(value, inner_ty);
        }
        else
        {
            // Not representable as the optional's inner type.
            return nullptr;
        }
    }

    // value -> { i1 true, value }
    llvm::Value* wrapped = llvm::UndefValue::get(optional_ty);
    wrapped = builder->CreateInsertValue(
        wrapped,
        builder->getInt1(OPT_HAS_VALUE),
        {OPT_IDX_HAS_VALUE}
    );
    wrapped = builder->CreateInsertValue(
        wrapped,
        value,
        {OPT_IDX_ELEMENT_TYPE}
    );
    return wrapped;
}

/// Extracts the second element from the wrapped type
/// If it's not wrapped, we just return the value as-is.
llvm::Value* stride::ast::unwrap_optional_value(llvm::Value* value, llvm::IRBuilder<>* builder)
{
    // If it's not an optional, we don't have to upwrap it.
    if (!is_optional_wrapped_type(value->getType()))
    {
        return value;
    }

    return builder->CreateExtractValue(
        value,
        {OPT_IDX_ELEMENT_TYPE},
        "unwrap_optional_ret"
    );
}
