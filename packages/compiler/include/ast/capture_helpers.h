#pragma once

#include <string>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/ValueSymbolTable.h>

namespace stride::ast::helpers
{
    /**
     * Looks up a variable in the function's symbol table, checking both regular and captured forms.
     * For captured variables, checks for both the original name and the __capture_ prefixed version.
     *
     * @param function The LLVM function to search in
     * @param internal_name The internal name of the variable to look up
     * @return The LLVM value if found, nullptr otherwise
     */
    inline llvm::Value* lookup_variable_or_capture(
        llvm::Function* function,
        const std::string& internal_name
    )
    {
        if (!function)
        {
            return nullptr;
        }

        llvm::ValueSymbolTable* symbol_table = function->getValueSymbolTable();

        // First try direct lookup
        if (llvm::Value* val = symbol_table->lookup(internal_name))
        {
            return val;
        }

        // Try captured variable form
        const std::string capture_name = "__capture_" + internal_name;
        return symbol_table->lookup(capture_name);
    }

    /**
     * Loads a captured variable value from the current function's context.
     * Handles both AllocaInst (for regular loads) and direct Arguments.
     *
     * @param builder The IR builder
     * @param capture_name The name of the captured variable (without __capture_ prefix)
     * @return The loaded value if found, nullptr otherwise
     */
    inline llvm::Value* load_captured_variable(
        llvm::IRBuilder<>* builder,
        const std::string& capture_name
    )
    {
        if (llvm::BasicBlock* block = builder->GetInsertBlock())
        {
            if (llvm::Function* function = block->getParent())
            {
                llvm::Value* captured_val = function->getValueSymbolTable()->lookup(capture_name);

                if (captured_val)
                {
                    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(captured_val))
                    {
                        return builder->CreateLoad(
                            alloca->getAllocatedType(),
                            alloca,
                            capture_name
                        );
                    }
                    return captured_val;
                }
            }
        }
        return nullptr;
    }
} // namespace stride::ast::helpers
