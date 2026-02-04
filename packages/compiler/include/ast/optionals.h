#pragma once
#include <llvm/IR/Value.h>
#include "parsing_context.h"

/**
 * Optional type handling
 *
 * Optional types have the following data layout in memory:
 * <code>
 * [ i1, void* ]
 * </code>
 * Here, <code>i1</code> represents whether the optional has a value, and <code>void*</code> is a pointer to the value.
 */
namespace stride::ast
{
#define OPT_NO_VALUE (0)
#define OPT_HAS_VALUE (1)

#define OPT_HAS_VALUE_BIT_COUNT (1)

#define OPT_IDX_HAS_VALUE (0)
#define OPT_IDX_ELEMENT_TYPE (1)

#define OPT_ELEMENT_COUNT (2)

    /**
     * Extracts the "has_value" part from the provided value type and returns
     * an optional containing the state. If the provided value doesn't conform to the
     * optional data layout, then it will return nullopt.
     */
    std::optional<bool> extract_has_value_from_optional(llvm::Value* optional, llvm::IRBuilder<>* builder);

    /**
     * Will set the "has_value" part to the provided state in <code>optional</code>.
     * If the provided value doesn't conform ot the optional data layout, then <code>optional</code>
     * will be returned as-is.
     */
    llvm::Value* set_has_value_in_optional_gep(llvm::Value* optional, llvm::IRBuilder<>* builder, bool has_value);

    /**
     * Checks if the provided LLVM type is an optional type.
     */
    bool is_optional_wrapped_type(const llvm::Type* type);

    /**
     * Wraps an LLVM value into an optional LLVM value.
     */
    llvm::Value* wrap_optional_value(
        llvm::Value* value,
        llvm::Type* optional_ty,
        llvm::IRBuilder<>* builder
    );

    llvm::Value* wrap_optional_value_gep(
        const std::string& name,
        llvm::Value* value,
        llvm::Type* optional_ty,
        const llvm::Module* module,
        llvm::IRBuilder<>* builder
    );

    /**
     * Unwraps (extracts) an optional LLVM value, returning the inner value if present, or nullptr if not.
     */
    llvm::Value* unwrap_optional_value(llvm::Value* value, llvm::IRBuilder<>* builder);

    llvm::Value* optionally_upcast_type(llvm::Value* value, llvm::Type* target_ty, llvm::IRBuilder<>* builder);
}
