#include "errors.h"
#include "formatting.h"
#include "ast/casting.h"
#include "ast/closures.h"
#include "ast/flags.h"
#include "ast/optionals.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <iostream>
#include <sstream>
#include <vector>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/TargetParser/Triple.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstExpression> stride::ast::parse_function_call(
    const std::shared_ptr<ParsingContext>& context,
    const SymbolNameSegments& function_name_segments,
    TokenSet& set
)
{
    const auto reference_token = set.peek(-1);
    auto function_parameter_set = collect_parenthesized_block(set);

    ExpressionList function_arg_nodes = {};

    int function_call_flags = SRFLAG_NONE;

    // Parsing function parameter values
    if (function_parameter_set.has_value())
    {
        auto subset = function_parameter_set.value();

        if (auto initial_arg = parse_inline_expression(context, subset))
        {
            function_arg_nodes.push_back(std::move(initial_arg));

            // Consume next parameters
            while (subset.has_next())
            {
                const auto preceding = subset.expect(
                    TokenType::COMMA,
                    "Expected ',' between function arguments"
                );

                auto function_argument = parse_inline_expression(context, subset);

                if (!function_argument)
                {
                    // Since the RParen is already consumed, we have to manually extract its
                    // position with the following assumption It's possible this yields END_OF_FILE
                    const auto len =
                        set.peek(-1).get_source_fragment().offset - 1 -
                        preceding.get_source_fragment().offset;
                    throw parsing_error(
                        ErrorType::SYNTAX_ERROR,
                        "Expected expression for function argument",
                        SourceFragment(
                            subset.get_source(),
                            preceding.get_source_fragment().offset + 1,
                            len)
                    );
                }

                // If the next argument is a variadic argument reference, we stop parsing more arguments and mark this function call as variadic
                if (cast_expr<AstVariadicArgReference*>(function_argument.get()))
                {
                    if (subset.has_next())
                    {
                        subset.throw_error(
                            "Variadic argument propagation must be the last parameter in a function call"
                        );
                    }
                    function_call_flags |= SRFLAG_FN_TYPE_VARIADIC;

                    break;
                }

                function_arg_nodes.push_back(std::move(function_argument));
            }
        }
    }

    const auto& ref_pos = reference_token.get_source_fragment();

    // TODO: Fix symbol position, as this is now incorrect for scoped variables.
    return std::make_unique<AstFunctionCall>(
        context,
        resolve_internal_name(
            /* context_name = */"",
                                ref_pos,
                                function_name_segments
        ),
        std::move(function_arg_nodes),
        function_call_flags
    );
}

std::string AstFunctionCall::format_suggestion(const IDefinition* suggestion)
{
    if (const auto fn_call = dynamic_cast<const FunctionDefinition*>(suggestion))
    {
        // We'll format the arguments
        std::vector<std::string> arg_types;

        for (const auto& arg : fn_call->get_type()->get_parameter_types())
        {
            arg_types.push_back(arg->get_type_name());
        }

        if (arg_types.empty())
            arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));

        return std::format("{}({})",
                           fn_call->get_symbol().name,
                           join(arg_types, ", "));
    }

    return suggestion->get_internal_symbol_name();
}

std::string AstFunctionCall::format_function_name() const
{
    std::vector<std::string> arg_types;

    arg_types.reserve(this->_arguments.size());

    for (const auto& arg : this->_arguments)
    {
        arg_types.push_back(arg->get_type()->to_string());
    }

    if (arg_types.empty())
    {
        arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));
    }

    return std::format("{}({})", this->get_function_name(), join(arg_types, ", "));
}

llvm::Value* AstFunctionCall::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Function* callee = nullptr;

    if (const auto definition =
            this->get_context()->get_function_definition(this->get_scoped_function_name(), this->get_argument_types());
        definition.has_value())
    {
        callee = module->getFunction(definition.value()->get_internal_symbol_name());

        // If the symbol exists in the semantic context but not yet in the IR module,
        // declare it as an external function now.
        if (!callee)
        {
            const auto fn_def = definition.value();
            const auto fn_type = fn_def->get_type();
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
            bool llvm_is_variadic;
            if (this->is_variadic())
            {
                param_types.push_back(llvm::PointerType::get(module->getContext(), 0));
                llvm_is_variadic = false;
            }
            else
            {
                llvm_is_variadic = (fn_def->get_flags() & SRFLAG_FN_TYPE_VARIADIC) != 0;
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
                fn_def->get_internal_symbol_name(),
                llvm_fn_type
            );

            callee = llvm::dyn_cast<llvm::Function>(callee_cand.getCallee());
        }
    }

    // Indirect call via a function-pointer variable (e.g. a variable holding a lambda).
    if (!callee)
    {
        if (auto* indirect_call = this->codegen_anonymous_function_call(module, builder))
        {
            return indirect_call;
        }
    }

    // No way to find it :(
    if (!callee)
    {
        for (const auto& defined_function : module->getFunctionList())
        {
            std::cout << defined_function.getName().str() << std::endl;
        }

        const auto suggested_alternative_symbol = this->get_context()->fuzzy_find(this->get_function_name());
        const auto suggested_alternative = suggested_alternative_symbol
            ? std::format("Did you mean '{}'?",
                          format_suggestion(suggested_alternative_symbol))
            : "";

        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Function '{}' was not found in this scope", this->format_function_name()),
            this->get_source_fragment(),
            suggested_alternative
        );
    }

    const auto minimum_arg_count = callee->arg_size();

    // When propagating varargs via '...', the va_list is appended as an extra argument
    // at codegen time, so the caller's declared arg count is one less than the callee expects.
    const auto effective_provided = this->get_arguments().size() + (this->is_variadic() ? 1 : 0);

    if (effective_provided < minimum_arg_count)
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
        llvm::Value* arg_val = nullptr;

        if (cast_expr<AstVariadicArgReference*>(arguments[i].get()))
        {
            // Do nothing, we'll append the va_list_ptr later if it exists
            continue;
        }
        else
        {
            arg_val = arguments[i]->codegen(module, builder);
        }

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
    if (const auto* var_def =
        this->get_context()->lookup_variable(this->get_scoped_function_name(), true))
    {
        auto base_type = var_def->get_type()->clone_ty();
        if (auto* alias_ty = cast_type<AstAliasType*>(base_type.get()))
        {
            base_type = alias_ty->get_underlying_type()->clone_ty();
        }

        if (const auto* fn_type = dynamic_cast<AstFunctionType*>(base_type.get()))
        {
            // First: check if the variable's internal name maps to a named function in the
            // symbol table with a matching type signature. If so, call it directly without
            // any pointer indirection.
            if (this->get_context()->get_function_definition(
                var_def->get_internal_symbol_name(),
                var_def->get_type()).has_value())
            {
                if (llvm::Function* callee =
                    module->getFunction(var_def->get_internal_symbol_name()))
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
            const auto fn_ptr = var_def->get_internal_symbol_name();
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
                    // Use the lambda's actual function type which includes captures
                    call_fn_type = lambda_fn->getFunctionType();

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

void AstFunctionCall::validate()
{
    for (const auto& arg : this->_arguments)
    {
        arg->validate();
    }
}

std::vector<std::unique_ptr<IAstType>> AstFunctionCall::get_argument_types() const
{
    if (this->_arguments.empty())
        return {};

    std::vector<std::unique_ptr<IAstType>> param_types;
    param_types.reserve(this->_arguments.size());
    for (const auto& arg : this->_arguments)
    {
        // The last parameter should be the variadic argument reference,
        // which is not included in the type list, as this is dynamically expanded
        // Additionally, this would mess with function lookup by signature
        if (cast_expr<AstVariadicArgReference*>(arg.get()))
        {
            break;
        }
        param_types.push_back(arg->get_type()->clone_ty());
    }
    return param_types;
}

std::unique_ptr<IAstNode> AstFunctionCall::clone()
{
    ExpressionList cloned_args;
    cloned_args.reserve(this->get_arguments().size());
    for (const auto& arg : this->get_arguments())
    {
        cloned_args.push_back(arg->clone_as<IAstExpression>());
    }

    return std::make_unique<AstFunctionCall>(
        this->get_context(),
        this->_symbol,
        std::move(cloned_args),
        this->_flags
    );
}

bool AstFunctionCall::is_reducible()
{
    // TODO: implement
    // Function calls can be reducible if the function returns
    // a constant value or if all arguments are reducible.
    return false;
}

std::optional<std::unique_ptr<IAstNode>> AstFunctionCall::reduce()
{
    return std::nullopt;
}

std::string AstFunctionCall::get_formatted_call() const
{
    std::vector<std::string> arg_names;
    arg_names.reserve(this->_arguments.size());

    for (const auto& arg : this->_arguments)
    {
        arg_names.push_back(arg->get_type()->get_type_name());
    }

    return std::format(
        "{}({})",
        this->get_function_name(),
        join(arg_names, ", ")
    );
}

std::string AstFunctionCall::to_string()
{
    std::ostringstream oss;

    std::vector<std::string> arg_types;
    for (const auto& arg : this->get_arguments())
    {
        arg_types.push_back(arg->to_string());
    }

    return std::format(
        "FunctionCall({} [{}])",
        this->get_function_name(),
        join(arg_types, ", ")
    );
}
