#pragma once
#include <llvm/IR/Value.h>
#include "symbol_registry.h"

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
     * Checks if the provided LLVM type is an optional type.
     */
    bool is_optional_wrapped_type(const llvm::Type* type);

    /**
     * Wraps an LLVM value into an optional LLVM value.
     */
    llvm::Value* wrap_into_optional_value(
        llvm::Value* value,
        const llvm::Module* module,
        llvm::IRBuilder<>* builder
    );

    /**
     * Unwraps (extracts) an optional LLVM value, returning the inner value if present, or nullptr if not.
     */
    llvm::Value* unwrap_optional_value(llvm::Value* value, llvm::IRBuilder<>* builder);
}
