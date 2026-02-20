#include "ast/optionals.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;

/// Checks whether the provided type conforms to the required data layout
bool stride::ast::is_optional_wrapped_type(const llvm::Type* type)
{
    if (const auto* struct_type = llvm::dyn_cast<llvm::StructType>(type))
    {
        if (struct_type->getNumElements() != OPT_ELEMENT_COUNT)
            return false;

        const auto has_value_ty = struct_type->
            getElementType(OPT_IDX_HAS_VALUE);
        const auto element_ty =
            struct_type->getElementType(OPT_IDX_ELEMENT_TYPE);
        // Type "T"; can be ptr or primitive.

        return has_value_ty != nullptr && element_ty != nullptr &&
            has_value_ty->isIntegerTy(OPT_HAS_VALUE_BIT_COUNT);
    }
    return false;
}

llvm::Value* stride::ast::wrap_optional_value(
    llvm::Value* value,
    llvm::Type* optional_ty,
    llvm::IRBuilder<>* builder)
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

    llvm::Type* inner_ty =
        llvm::cast<llvm::StructType>(optional_ty)->getElementType(
            OPT_IDX_ELEMENT_TYPE);

    // nil -> { i1 false, undef inner }
    if (llvm::isa<llvm::ConstantPointerNull>(value))
    {
        llvm::Value* wrapped = llvm::UndefValue::get(optional_ty);
        wrapped = builder->CreateInsertValue(
            wrapped,
            builder->getInt1(OPT_NO_VALUE),
            { OPT_IDX_HAS_VALUE });
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
    wrapped =
        builder->CreateInsertValue(wrapped,
                                   builder->getInt1(OPT_HAS_VALUE),
                                   { OPT_IDX_HAS_VALUE });
    wrapped = builder->CreateInsertValue(wrapped,
                                         value,
                                         { OPT_IDX_ELEMENT_TYPE });
    return wrapped;
}

llvm::Value* stride::ast::optionally_upcast_type(
    llvm::Value* value,
    llvm::Type* target_ty,
    llvm::IRBuilder<>* builder)
{
    const auto value_ty = value->getType();

    if (value_ty == target_ty)
    {
        return value;
    }
    if (value_ty->isIntegerTy() && target_ty->isIntegerTy())
    {
        return builder->CreateIntCast(value, target_ty, true);
    }

    if (value_ty->isFloatTy() && target_ty->isFloatTy())
    {
        return builder->CreateFPExt(value, target_ty, "fpext");
    }

    if (value_ty->isPointerTy() && target_ty->isPointerTy())
    {
        return builder->CreatePointerCast(value, target_ty);
    }


    return value;
}

llvm::Value* stride::ast::wrap_optional_value_gep(
    const std::string& name,
    llvm::Value* value,
    llvm::Type* optional_ty,
    const llvm::Module* module,
    llvm::IRBuilder<>* builder)
{
    const auto value_ty = value->getType();

    if (!is_optional_wrapped_type(optional_ty) || is_optional_wrapped_type(
        value_ty))
    {
        return value;
    }

    llvm::AllocaInst* alloca = builder->
        CreateAlloca(optional_ty, nullptr, name);

    // If both types are the same, we can store directly.
    if (value_ty == optional_ty)
    {
        builder->CreateStore(
            optionally_upcast_type(value, optional_ty, builder),
            alloca);
        return alloca;
    }

    auto& ctx = module->getContext();

    llvm::Value* opt_has_value = nullptr;
    llvm::Value* opt_value = nullptr;

    const auto optional_value_ty = optional_ty->getStructElementType(
        OPT_IDX_ELEMENT_TYPE);

    if (llvm::isa<llvm::ConstantPointerNull>(value))
    {
        opt_has_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx),
                                               OPT_NO_VALUE);
        opt_value = llvm::UndefValue::get(optional_ty);
    }
    else
    {
        opt_has_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx),
                                               OPT_HAS_VALUE);
        opt_value = optionally_upcast_type(value, optional_value_ty, builder);
    }

    llvm::Value* has_value_ptr =
        builder->CreateStructGEP(alloca->getAllocatedType(),
                                 alloca,
                                 OPT_IDX_HAS_VALUE);
    builder->CreateStore(opt_has_value, has_value_ptr);

    // Store value
    llvm::Value* value_ptr =
        builder->CreateStructGEP(alloca->getAllocatedType(),
                                 alloca,
                                 OPT_IDX_ELEMENT_TYPE);
    builder->CreateStore(opt_value, value_ptr);

    return alloca;
}

/// Extracts the second element from the wrapped type
/// If it's not wrapped, we just return the value as-is.
llvm::Value* stride::ast::unwrap_optional_value(llvm::Value* value,
                                                llvm::IRBuilder<>* builder)
{
    // If it's not an optional, we don't have to upwrap it.
    if (!is_optional_wrapped_type(value->getType()))
    {
        return value;
    }

    return builder->CreateExtractValue(value,
                                       { OPT_IDX_ELEMENT_TYPE },
                                       "unwrap_optional_val");
}

std::optional<bool> stride::ast::extract_has_value_from_optional(
    llvm::Value* optional,
    llvm::IRBuilder<>* builder)
{
    if (!optional || !is_optional_wrapped_type(optional->getType()))
    {
        return std::nullopt;
    }


    return builder->CreateExtractValue(optional,
                                       { OPT_IDX_HAS_VALUE },
                                       "unwrap_optional_state");
}

llvm::Value* stride::ast::set_has_value_in_optional_gep(
    llvm::Value* optional,
    llvm::IRBuilder<>* builder,
    const bool has_value)
{
    if (!is_optional_wrapped_type(optional->getType()))
        return optional;

    return builder->CreateInsertValue(optional,
                                      builder->getInt1(has_value),
                                      { OPT_IDX_HAS_VALUE });
}
