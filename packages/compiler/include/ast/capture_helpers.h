#pragma once

#include <string>
#include <vector>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#define ANONYMOUS_FN_PREFIX "#__anonymous_"

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
    llvm::Value* lookup_variable_or_capture(
        llvm::Function* function,
        const std::string& internal_name
    );

    /**
     * Looks up a variable by its base name (without the numeric suffix).
     * Searches for variables matching pattern: base_name.N or __capture_base_name.N
     * where N is any numeric suffix added by the compiler.
     *
     * @param function The LLVM function to search in
     * @param base_name The base name of the variable (e.g., "factor" for "factor.0")
     * @return The LLVM value if found, nullptr otherwise
     */
    llvm::Value* lookup_variable_by_base_name(
        llvm::Function* function,
        const std::string& base_name
    );

    /**
     * Loads a captured variable value from the current function's context.
     * Handles both AllocaInst (for regular loads) and direct Arguments.
     *
     * @param builder The IR builder
     * @param capture_name The name of the captured variable (without __capture_ prefix)
     * @return The loaded value if found, nullptr otherwise
     */
    llvm::Value* load_captured_variable(
        llvm::IRBuilder<>* builder,
        const std::string& capture_name
    );

    /**
     * Finds the lambda function that corresponds to a function pointer variable.
     * Matches by function type signature.
     *
     * @param module The LLVM module to search in
     * @param fn_type The function type to match
     * @return The lambda function if found, nullptr otherwise
     */
    llvm::Function* find_lambda_function(
        llvm::Module* module,
        const llvm::FunctionType* fn_type
    );

    /**
     * Generates the captured variable arguments for a lambda function call.
     * Extracts capture parameter names from the lambda function and looks up
     * their values in the current scope.
     *
     * @param module The LLVM module
     * @param builder The IR builder
     * @param lambda_fn The lambda function being called
     * @param num_declared_params The number of declared parameters (non-captured)
     * @return Vector of captured variable values in the correct order
     */
    std::vector<llvm::Value*> generate_capture_arguments(
        llvm::Module* module,
        llvm::IRBuilder<>* builder,
        llvm::Function* lambda_fn,
        size_t num_declared_params
    );
} // namespace stride::ast::helpers
