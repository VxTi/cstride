#include "ast/capture_helpers.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

namespace stride::ast::helpers
{
    llvm::Value* lookup_variable_or_capture(
        llvm::Function* function,
        const std::string& internal_name
    )
    {
        if (!function)
        {
            return nullptr;
        }

        const llvm::ValueSymbolTable* symbol_table = function->getValueSymbolTable();

        // First try direct lookup
        if (llvm::Value* val = symbol_table->lookup(internal_name))
        {
            return val;
        }

        // Try captured variable form
        const std::string capture_name = "__capture_" + internal_name;
        return symbol_table->lookup(capture_name);
    }

    llvm::Value* lookup_variable_by_base_name(
        llvm::Function* function,
        const std::string& base_name
    )
    {
        if (!function)
        {
            return nullptr;
        }

        const llvm::ValueSymbolTable* symbol_table = function->getValueSymbolTable();

        // Search for a variable matching the pattern: base_name + "." + digit(s)
        // e.g., "factor.0", "factor.1", "factor.2", etc.
        for (const auto& entry : *symbol_table)
        {
            const std::string name = std::string(entry.first());

            // Check if name starts with base_name followed by a dot
            if (name.starts_with(base_name + "."))
            {
                return entry.second;
            }

            // Also check for captured variable form: __capture_base_name.N
            if (name.starts_with("__capture_" + base_name + "."))
            {
                return entry.second;
            }
        }

        return nullptr;
    }

    llvm::Value* load_captured_variable(
        llvm::IRBuilder<>* builder,
        const std::string& capture_name
    )
    {
        if (llvm::BasicBlock* block = builder->GetInsertBlock())
        {
            if (llvm::Function* function = block->getParent())
            {
                if (llvm::Value* captured_val = function->getValueSymbolTable()->lookup(capture_name))
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

    llvm::Function* find_lambda_function(
        llvm::Module* module,
        const llvm::FunctionType* fn_type
    )
    {
        if (!module || !fn_type)
        {
            return nullptr;
        }

        // The lambda function has extra parameters for captured variables at the beginning
        // We need to match by:
        // 1. Return type must match
        // 2. The LAST N parameters must match (where N = declared params)
        // 3. The lambda should have >= N parameters

        for (auto& fn : module->functions())
        {
            if (fn.getName().starts_with(ANONYMOUS_FN_PREFIX))
            {
                const llvm::FunctionType* lambda_type = fn.getFunctionType();

                // Check if lambda has at least as many params as declared
                if (lambda_type->getNumParams() < fn_type->getNumParams())
                {
                    continue;
                }

                // Check return type match
                if (lambda_type->getReturnType() != fn_type->getReturnType())
                {
                    continue;
                }

                // Check if the LAST N parameters match the declared parameters
                const size_t num_declared = fn_type->getNumParams();
                const size_t lambda_params = lambda_type->getNumParams();
                const size_t capture_offset = lambda_params - num_declared;

                bool params_match = true;
                for (size_t i = 0; i < num_declared; ++i)
                {
                    if (lambda_type->getParamType(capture_offset + i) != fn_type->getParamType(i))
                    {
                        params_match = false;
                        break;
                    }
                }

                if (params_match)
                {
                    return &fn;
                }
            }
        }
        return nullptr;
    }

    std::vector<llvm::Value*> generate_capture_arguments(
        llvm::Module* module,
        llvm::IRBuilder<>* builder,
        llvm::Function* lambda_fn,
        const size_t num_declared_params
    )
    {
        std::vector<llvm::Value*> capture_args;

        if (!lambda_fn || !builder)
        {
            return capture_args;
        }

        // Calculate number of captured variables
        const size_t num_captures = lambda_fn->arg_size() - num_declared_params;

        auto arg_it = lambda_fn->arg_begin();
        for (size_t i = 0; i < num_captures; ++i, ++arg_it)
        {
            // Extract the original variable name from the parameter name
            // Parameter is named like "factor.0.capture"
            std::string param_name = std::string(arg_it->getName());
            size_t dot_pos = param_name.find('.');
            std::string var_name = (dot_pos != std::string::npos)
                ? param_name.substr(0, dot_pos)
                : param_name;

            // Look up the captured variable in the current scope by its base name
            // This will find variables with any numeric suffix (e.g., "factor.0", "factor.1", etc.)
            llvm::Value* captured_val = nullptr;
            if (const auto block = builder->GetInsertBlock())
            {
                llvm::Function* current_fn = block->getParent();
                captured_val = lookup_variable_by_base_name(current_fn, var_name);

                if (captured_val)
                {
                    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(captured_val))
                    {
                        captured_val = builder->CreateLoad(
                            alloca->getAllocatedType(),
                            alloca,
                            var_name
                        );
                    }
                }
            }

            if (captured_val)
            {
                capture_args.push_back(captured_val);
            }
        }

        return capture_args;
    }

    llvm::Value* create_closure(
        llvm::Module* module,
        llvm::IRBuilder<>* builder,
        llvm::Function* lambda_fn,
        const std::vector<llvm::Value*>& captured_values
    )
    {
        // If no captures, just return the function pointer directly
        if (captured_values.empty())
        {
            return lambda_fn;
        }

        // Create malloc declaration if it doesn't exist
        llvm::Function* malloc_fn = module->getFunction("malloc");
        if (!malloc_fn)
        {
            llvm::FunctionType* malloc_type = llvm::FunctionType::get(
                llvm::PointerType::getUnqual(module->getContext()),  // returns void*
                {llvm::Type::getInt64Ty(module->getContext())},       // takes size_t
                false
            );
            malloc_fn = llvm::Function::Create(
                malloc_type,
                llvm::Function::ExternalLinkage,
                "malloc",
                module
            );
        }

        // Calculate total size needed: 1 pointer (function) + N values (captures)
        // Size = sizeof(ptr) + sum of sizeof(each captured value)
        uint64_t total_size = module->getDataLayout().getPointerSize();
        for (llvm::Value* val : captured_values)
        {
            total_size += module->getDataLayout().getTypeAllocSize(val->getType());
        }

        // Allocate closure structure on heap
        llvm::Value* closure_ptr = builder->CreateCall(
            malloc_fn,
            {llvm::ConstantInt::get(llvm::Type::getInt64Ty(module->getContext()), total_size)}
        );

        // Cast to appropriate pointer type for storing function pointer
        llvm::Value* fn_ptr_slot = builder->CreatePointerCast(
            closure_ptr,
            llvm::PointerType::getUnqual(module->getContext())
        );

        // Store the function pointer at offset 0
        builder->CreateStore(lambda_fn, fn_ptr_slot);

        // Store each captured value after the function pointer
        uint64_t offset = module->getDataLayout().getPointerSize();
        for (size_t i = 0; i < captured_values.size(); ++i)
        {
            llvm::Value* capture_val = captured_values[i];
            llvm::Type* capture_type = capture_val->getType();

            // Calculate pointer to this capture slot
            llvm::Value* capture_slot = builder->CreateConstGEP1_64(
                llvm::Type::getInt8Ty(module->getContext()),
                closure_ptr,
                offset
            );

            // Cast to appropriate type
            llvm::Value* typed_slot = builder->CreatePointerCast(
                capture_slot,
                llvm::PointerType::getUnqual(module->getContext())
            );

            // Store the captured value
            builder->CreateStore(capture_val, typed_slot);

            offset += module->getDataLayout().getTypeAllocSize(capture_type);
        }

        // Return the closure pointer cast to function pointer type for compatibility
        return builder->CreatePointerCast(closure_ptr, lambda_fn->getType());
    }

    std::vector<llvm::Value*> extract_closure_captures(
        const llvm::Module* module,
        llvm::IRBuilder<>* builder,
        llvm::Value* fn_ptr_val,
        const size_t num_captures
    )
    {
        std::vector<llvm::Value*> captures;

        if (num_captures == 0)
        {
            return captures;
        }

        // Cast the function pointer back to generic pointer
        llvm::Value* closure_ptr = builder->CreatePointerCast(
            fn_ptr_val,
            llvm::PointerType::getUnqual(module->getContext())
        );

        // Skip the function pointer (first slot) and extract captured values
        uint64_t offset = module->getDataLayout().getPointerSize();

        // We need to know the types of the captures to extract them properly
        // For now, we'll extract them as i32 values (this is a simplification)
        // In a full implementation, we'd need to store type information in the closure
        for (size_t i = 0; i < num_captures; ++i)
        {
            llvm::Value* capture_slot = builder->CreateConstGEP1_64(
                llvm::Type::getInt8Ty(module->getContext()),
                closure_ptr,
                offset
            );

            // For now, assume all captures are i32 (this needs to be improved)
            llvm::Type* capture_type = llvm::Type::getInt32Ty(module->getContext());
            llvm::Value* typed_slot = builder->CreatePointerCast(
                capture_slot,
                llvm::PointerType::getUnqual(module->getContext())
            );

            llvm::Value* loaded_val = builder->CreateLoad(capture_type, typed_slot);
            captures.push_back(loaded_val);

            offset += module->getDataLayout().getTypeAllocSize(capture_type);
        }

        return captures;
    }
} // namespace stride::ast::helpers
