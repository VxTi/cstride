#include "ast/casting.h"
#include "ast/closures.h"
#include "ast/optionals.h"
#include "ast/parsing_context.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

llvm::Value* AstFunctionCall::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    if (llvm::Function* callee = this->resolve_regular_callee(module))
    {
        return this->codegen_regular_function_call(callee, module, builder);
    }

    // Indirect call via a function-pointer variable (e.g. a variable holding a lambda).
    if (auto* indirect_call = this->codegen_anonymous_function_call(module, builder))
    {
        return indirect_call;
    }

    const auto suggested_alternative_symbol = this->get_context()->fuzzy_find(this->get_function_name());
    const auto suggested_alternative = suggested_alternative_symbol != nullptr
        ? std::format("Did you mean '{}'?", format_suggestion(suggested_alternative_symbol))
        : "";

    throw parsing_error(
        ErrorType::COMPILATION_ERROR,
        std::format("Function '{}' was not found in this scope", this->format_function_name()),
        this->get_source_fragment(),
        suggested_alternative
    );
}

llvm::Function* AstFunctionCall::resolve_regular_callee(llvm::Module* module)
{
    const auto& definition = this->get_function_definition();

    const AstFunctionType* fn_type = nullptr;

    if (const auto* fn_definition = dynamic_cast<definition::FunctionDefinition*>(definition))
    {
        fn_type = fn_definition->get_type();
        if (this->_generic_type_arguments.empty())
        {
            if (llvm::Function* callee = fn_definition->get_llvm_function())
            {
                return callee;
            }
        }
        else if (auto* llvm_func = fn_definition->get_generic_overload_llvm_function(this->_generic_type_arguments))
        {
            return llvm_func;
        }
    } else if (dynamic_cast<definition::FieldDefinition*>(definition))
    {
        // Callable variables (function pointers, lambdas) are handled by
        // codegen_anonymous_function_call, not as regular callees.
        return nullptr;
    }

    if (fn_type == nullptr)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Function '{}' is not a function", this->format_function_name()),
            this->get_source_fragment()
        );
    }

    std::vector<llvm::Type*> param_types;
    param_types.reserve(fn_type->get_parameter_types().size());

    for (const auto& param : fn_type->get_parameter_types())
    {
        param_types.push_back(param->get_llvm_type(module));
    }

    llvm::Type* ret_type = fn_type->get_return_type()->get_llvm_type(module);

    // When propagating varargs (call has '...'), the callee receives the caller's
    // va_list as an extra fixed pointer argument rather than as true variadic args.
    // This lets the callee forward the va_list directly to vprintf/vscanf-style APIs.
    bool llvm_is_variadic = false;
    if (this->is_variadic())
    {
        param_types.push_back(llvm::PointerType::get(module->getContext(), 0));
    }
    else
    {
        llvm_is_variadic = fn_type->is_variadic();
    }

    llvm::FunctionType* llvm_fn_type = llvm::FunctionType::get(
        ret_type,
        param_types,
        llvm_is_variadic
    );

    // If we are calling a variadic function and propagating '...',
    // the callee is actually a non-variadic function that takes a va_list.
    // But we should use the actual function name for the lookup.
    auto callee_cand = module->getOrInsertFunction(
        definition->get_internal_symbol_name(),
        llvm_fn_type
    );

    return llvm::dyn_cast<llvm::Function>(callee_cand.getCallee());
}

llvm::Value* AstFunctionCall::codegen_regular_function_call(
    llvm::Function* callee,
    llvm::Module* module,
    llvm::IRBuilderBase* builder
) const
{
    const auto minimum_arg_count = callee->arg_size();

    // When propagating varargs via '...', the va_list is appended as an extra argument
    // at codegen time, so the caller's declared arg count is one less than the callee expects.

    if (const auto effective_provided = this->get_arguments().size() + (this->is_variadic() ? 1 : 0);
        effective_provided < minimum_arg_count)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Incorrect arguments passed for function '{}', expected {}, got {}",
                        this->get_function_name(),
                        minimum_arg_count,
                        effective_provided),
            this->get_source_fragment()
        );
    }

    std::vector<llvm::Value*> args_v;
    const auto& arguments = this->get_arguments();

    llvm::Value* va_list_ptr = nullptr;
    if (this->is_variadic())
    {
        va_list_ptr = AstVariadicArgReference::init_variadic_reference(module, builder);
    }

    for (size_t i = 0; i < arguments.size(); ++i)
    {

        if (cast_expr<AstVariadicArgReference*>(arguments[i].get()))
        {
            // Do nothing, we'll append the va_list_ptr later if it exists
            break;
        }

        llvm::Value* arg_val = arguments[i]->codegen(module, builder);

        if (!arg_val)
        {
            return nullptr;
        }

        llvm::Value* final_val = arg_val;

        // Determine if we need to unwrap based on the target function signature
        if (i < callee->arg_size())
        {
            // Check for strict type equality.
            // If the argument is Optional<T> but the function expects T, we unwrap.
            if (const llvm::Type* expected_type = callee->getFunctionType()->getParamType(i);
                arg_val->getType() != expected_type)
            {
                final_val = unwrap_optional_value(arg_val, builder);
            }
        }
        else
        {
            // For variadic arguments, we default to eager unwrapping
            final_val = unwrap_optional_value(arg_val, builder);
        }

        args_v.push_back(final_val);
    }

    if (va_list_ptr)
    {
        // On Darwin ARM64, va_list is char* (a single pointer value), not a struct.
        // vprintf expects that char* value in a register, so we must load it from
        // the alloca rather than passing the alloca's address.
        if (const llvm::Triple& triple(module->getTargetTriple());
            triple.getArch() == llvm::Triple::aarch64 && triple.isOSDarwin())
        {
            llvm::Value* va_list_val = builder->CreateLoad(
                llvm::PointerType::get(module->getContext(), 0),
                va_list_ptr,
                "va_list_val"
            );
            args_v.push_back(va_list_val);
        }
        else
        {
            args_v.push_back(va_list_ptr);
        }
    }

    const auto call_inst = builder->CreateCall(
        callee,
        args_v,
        callee->getReturnType()->isVoidTy() ? "" : "calltmp"
    );

    if (va_list_ptr)
    {
        AstVariadicArgReference::end_variadic_reference(module, builder, va_list_ptr);
    }

    return call_inst;
}

llvm::Value* AstFunctionCall::codegen_anonymous_function_call(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
) const
{
    // Variables are stored by their declaration name, not with function signatures
    // So we lookup using the raw function name only
    if (const auto var_def = this->get_function_name_identifier()->get_definition();
        var_def.has_value())
    {
        const auto field_def = dynamic_cast<definition::FieldDefinition*>(var_def.value());

        if (!field_def)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Anonymous function call to non-function '{}'",
                            this->get_function_name()),
                this->get_source_fragment()
            );
        }

        auto base_type = field_def->get_type()->clone_ty();
        if (auto* alias_ty = cast_type<AstAliasType*>(base_type.get()))
        {
            base_type = alias_ty->get_underlying_type()->clone_ty();
        }

        if (const auto* fn_type = dynamic_cast<AstFunctionType*>(base_type.get()))
        {
            // First: check if the variable's internal name maps to a named function in the
            // symbol table with a matching type signature. If so, call it directly without
            // any pointer indirection.
            // We sadly cannot use `get_function_definition` here
            if (this->get_context()->get_function_definition(
                field_def->get_internal_symbol_name(),
                field_def->get_type()).has_value())
            {
                if (llvm::Function* callee = module->getFunction(field_def->get_internal_symbol_name()))
                {
                    std::vector<llvm::Value*> args_v;
                    for (const auto& arg : this->get_arguments())
                    {
                        auto* arg_val = arg->codegen(module, builder);
                        if (!arg_val)
                            return nullptr;
                        args_v.push_back(unwrap_optional_value(arg_val, builder));
                    }
                    const auto instruction_name =
                        callee->getReturnType()->isVoidTy() ? "" : "indcalltmp";
                    return builder->CreateCall(callee, args_v, instruction_name);
                }
            }

            // Otherwise: assume the value is a function pointer and make a generic pointer call.

            // Validate argument count matches lambda signature
            const auto expected_param_count = fn_type->get_parameter_types().size();

            if (const auto provided_arg_count = this->get_arguments().size();
                provided_arg_count != expected_param_count)
            {
                throw parsing_error(
                    ErrorType::COMPILATION_ERROR,
                    std::format(
                        "Incorrect number of arguments for lambda call to '{}': expected {}, got {}",
                        this->get_function_name(),
                        expected_param_count,
                        provided_arg_count),
                    this->get_source_fragment(),
                    std::format("Lambda expects {} parameter(s)", expected_param_count)
                );
            }

            // Validate argument types match lambda signature
            for (size_t i = 0; i < expected_param_count; ++i)
            {
                const auto arg_type = this->get_arguments()[i]->get_type();

                if (const auto expected_type = fn_type->get_parameter_types()[i].get();
                    !arg_type->equals(expected_type))
                {
                    throw parsing_error(
                        ErrorType::TYPE_ERROR,
                        std::format(
                            "Type mismatch for argument {} in anonymous function call '{}': expected type '{}', got '{}'",
                            i + 1,
                            this->get_function_name(),
                            expected_type->get_type_name(),
                            arg_type->get_type_name()
                        ),
                        {
                            ErrorSourceReference(
                                std::format(
                                    "Expected type: {}",
                                    expected_type->get_type_name()
                                ),
                                arg_type->get_source_fragment()
                            )
                        }
                    );
                }
            }

            // AstFunctionType always sets SRFLAG_TYPE_PTR, so get_llvm_type() returns an opaque
            // PointerType rather than the underlying FunctionType. We must reconstruct the
            // llvm::FunctionType directly from the AST type's parameter/return types.
            std::vector<llvm::Type*> llvm_param_types;
            llvm_param_types.reserve(fn_type->get_parameter_types().size());
            for (const auto& param : fn_type->get_parameter_types())
            {
                llvm_param_types.push_back(param->get_llvm_type(module));
            }
            llvm::FunctionType* llvm_fn_type = llvm::FunctionType::get(
                fn_type->get_return_type()->get_llvm_type(module),
                llvm_param_types,
                fn_type->is_variadic()
            );

            if (!llvm_fn_type)
            {
                throw parsing_error(
                    ErrorType::COMPILATION_ERROR,
                    std::format("Invalid function type for anonymous function call '{}'",
                                this->get_function_name()),
                    this->get_source_fragment()
                );
            }

            // Load the function pointer from the variable
            const auto fn_ptr = field_def->get_internal_symbol_name();
            llvm::Value* fn_ptr_val = nullptr;

            if (const auto block = builder->GetInsertBlock())
            {
                llvm::Function* current_fn = block->getParent();
                // Use helper to lookup variable or capture
                fn_ptr_val = closures::lookup_variable_or_capture(current_fn, fn_ptr);

                if (fn_ptr_val)
                {
                    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(fn_ptr_val))
                    {
                        fn_ptr_val = builder->CreateLoad(
                            alloca->getAllocatedType(),
                            alloca,
                            fn_ptr
                        );
                    }
                }
            }

            // Maybe it's globally defined
            if (!fn_ptr_val)
            {
                fn_ptr_val = module->getNamedGlobal(fn_ptr);
                if (fn_ptr_val)
                {
                    auto* global = llvm::dyn_cast<llvm::GlobalVariable>(fn_ptr_val);
                    fn_ptr_val = builder->CreateLoad(
                        global->getValueType(),
                        global,
                        fn_ptr
                    );
                }
            }

            if (fn_ptr_val)
            {
                // Generate arguments for the lambda call
                std::vector<llvm::Value*> args_v;

                // Find the lambda function to determine if it has captured variables
                llvm::Function* lambda_fn =
                    closures::find_lambda_function(module, llvm_fn_type);

                // Determine the actual function type to use for the call
                llvm::FunctionType* call_fn_type = llvm_fn_type;
                llvm::Value* actual_fn_ptr = fn_ptr_val;

                // If we found the lambda function, handle captured variables
                if (lambda_fn)
                {
                    const size_t num_captures = lambda_fn->arg_size()
                        - fn_type->get_parameter_types().size();

                    if (num_captures > 0)
                    {
                        // This might be a closure - try to extract captures from it
                        auto capture_args = closures::extract_closure_captures(
                            module,
                            builder,
                            fn_ptr_val,
                            lambda_fn
                        );

                        // If closure extraction succeeded, extract the actual function pointer
                        if (capture_args.size() == num_captures)
                        {
                            // Use the lambda's actual function type which includes captures
                            call_fn_type = lambda_fn->getFunctionType();

                            // Extract the function pointer from the closure (first element)
                            llvm::Value* closure_ptr = builder->CreatePointerCast(
                                fn_ptr_val,
                                llvm::PointerType::get(module->getContext(), 0)
                            );
                            actual_fn_ptr = builder->CreateLoad(
                                lambda_fn->getType(),
                                closure_ptr,
                                "closure_fn_ptr"
                            );

                            args_v.insert(
                                args_v.end(),
                                capture_args.begin(),
                                capture_args.end());
                        }
                        else
                        {
                            // Try the old method: look up captures in the current scope
                            auto capture_args_old = closures::generate_capture_arguments(
                                builder,
                                lambda_fn,
                                fn_type->get_parameter_types().size()
                            );

                            if (capture_args_old.size() == num_captures)
                            {
                                call_fn_type = lambda_fn->getFunctionType();

                                args_v.insert(
                                    args_v.end(),
                                    capture_args_old.begin(),
                                    capture_args_old.end());
                            }
                        }
                    }
                }

                // Add the declared arguments
                for (const auto& arguments = this->get_arguments();
                     const auto& argument : arguments)
                {
                    auto* arg_val = argument->codegen(module, builder);
                    if (!arg_val)
                    {
                        return nullptr;
                    }

                    args_v.push_back(unwrap_optional_value(arg_val, builder));
                }

                const auto instruction_name =
                    call_fn_type->getReturnType()->isVoidTy() ? "" : "indcalltmp";
                return builder->CreateCall(
                    call_fn_type,
                    actual_fn_ptr,
                    args_v,
                    instruction_name
                );
            }
        }
    }

    return nullptr;
}
