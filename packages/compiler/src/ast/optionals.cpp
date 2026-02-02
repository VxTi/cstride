#include "ast/optionals.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;

static llvm::StructType* get_optional_struct_type(const llvm::Module* module)
{
    const auto internal_name = "stride.optional";

    auto& ctx = module->getContext();
    if (auto* existing_ty = llvm::StructType::getTypeByName(ctx, internal_name))
    {
        return llvm::cast<llvm::StructType>(existing_ty);
    }

    auto* opt_ty = llvm::StructType::create(ctx, internal_name);
    opt_ty->setBody(
        {
            llvm::Type::getInt1Ty(ctx),    // has_value
            llvm::PointerType::get(ctx, 0) // void *
        },
        /*isPacked = */ false
    );
    return opt_ty;
}

/// Checks whether the provided type conforms to the required data layout
bool stride::ast::is_optional_wrapped_type(const llvm::Type* type)
{
    if (const auto* struct_type = llvm::dyn_cast<llvm::StructType>(type))
    {
        if (struct_type->getNumElements() != OPT_ELEMENT_COUNT) return false;

        const auto element_ty = struct_type->getElementType(OPT_IDX_HAS_VALUE);
        const auto element_ptr_ty = struct_type->getElementType(OPT_IDX_ELEMENT_TYPE);

        return element_ty != nullptr && element_ty->isIntegerTy(OPT_HAS_VALUE_BIT_COUNT)
            && element_ptr_ty != nullptr && element_ptr_ty->isPointerTy();
    }
    return false;
}

llvm::Value* stride::ast::wrap_into_optional_value(
    llvm::Value* value,
    const llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    if (is_optional_wrapped_type(value->getType()))
    {
        // value is already a wrapped type; no work to do here.
        return value;
    }

    llvm::Type* optional_struct_ty = get_optional_struct_type(module);

    // If the value is `nullptr`, we can just simply return an empty
    // value.
    if (llvm::isa<llvm::ConstantPointerNull>(value))
    {
        //  Returns [0, undef]
        return builder->CreateInsertValue(
            llvm::UndefValue::get(optional_struct_ty),
            builder->getInt1(OPT_NO_VALUE),
            {OPT_IDX_HAS_VALUE}
        );
    }

    // Create value with layout [<has_value>(i1), <value>]
    llvm::Value* optional_value_ty = llvm::UndefValue::get(optional_struct_ty);
    optional_value_ty = builder->CreateInsertValue(
        optional_value_ty,
        builder->getInt1(OPT_HAS_VALUE),
        {OPT_IDX_HAS_VALUE}
    );
    optional_value_ty = builder->CreateInsertValue(
        optional_value_ty,
        value,
        {OPT_IDX_ELEMENT_TYPE}
    );
    return optional_value_ty; // Now contains the right data layout.
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
