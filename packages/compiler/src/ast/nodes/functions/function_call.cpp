#include "errors.h"
#include "formatting.h"
#include "ast/casting.h"
#include "ast/closures.h"
#include "ast/optionals.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <sstream>
#include <vector>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstExpression> stride::ast::parse_function_call(
    const std::shared_ptr<ParsingContext>& context,
    const SymbolNameSegments& function_name_segments,
    TokenSet& set)
{
    const auto reference_token = set.peek(-1);
    auto function_parameter_set = collect_parenthesized_block(set);

    std::vector<std::unique_ptr<IAstExpression>> function_arg_nodes = {};

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
                        set.at(set.position() - 1).get_source_fragment().offset - 1 -
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
                    break;
                }

                function_arg_nodes.push_back(std::move(function_argument));
            }
        }
    }

    const auto& ref_pos = reference_token.get_source_fragment();

    /* TODO: Fix this. Functions might not have parameters, in which case `back` returns a nullptr and segfaults here.
     const auto& last_pos = parameter_types.back()->get_source_fragment();
    auto position = SourceFragment(
        set.get_source(),
        ref_pos.offset,
        parameter_types.empty()
        ? ref_pos.length
        : last_pos.offset + last_pos.length - ref_pos.offset);*/

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
        arg_types.push_back(arg->get_type()->get_type_name());
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
            this->get_context()->get_function_definition(this->get_internal_name(), this->get_argument_types());
        definition.has_value())
    {
        callee = module->getFunction(this->get_internal_name());
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
        const auto suggested_alternative_symbol = this->get_context()->fuzzy_find(this->get_function_name());
        const auto suggested_alternative = suggested_alternative_symbol
            ? std::format("Did you mean '{}'?",
                          format_suggestion(suggested_alternative_symbol))
            : "";

        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Function '{}' was not found in this scope",
                        this->format_function_name()),
            this->get_source_fragment(),
            suggested_alternative);
    }

    const auto minimum_arg_count = callee->arg_size();

    if (const auto provided_arg_count = this->get_arguments().size();
        provided_arg_count < minimum_arg_count)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Incorrect arguments passed for function '{}', expected {}, got {}",
                        this->get_function_name(),
                        minimum_arg_count,
                        provided_arg_count),
            this->get_source_fragment()
        );
    }

    std::vector<llvm::Value*> args_v;
    const auto& arguments = this->get_arguments();

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        const auto arg_val = arguments[i]->codegen(module, builder);

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

    return builder->CreateCall(
        callee,
        args_v,
        callee->getReturnType()->isVoidTy() ? "" : "calltmp"
    );
}

llvm::Value* AstFunctionCall::codegen_anonymous_function_call(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
) const
{
    // Variables are stored by their declaration name, not with function signatures
    // So we lookup using the raw function name only
    if (const auto* var_def = this->get_context()->lookup_variable(
        this->get_function_name(),
        true))
    {
        if (const auto* fn_type = cast_type<AstFunctionType*>(var_def->get_type()))
        {
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
                    arg_type->get_type_name() != expected_type->get_type_name())
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

            // Reconstruct the concrete llvm::FunctionType from the AST type
            std::vector<llvm::Type*> param_types;
            param_types.reserve(fn_type->get_parameter_types().size());

            for (const auto& param : fn_type->get_parameter_types())
            {
                param_types.push_back(type_to_llvm_type(param.get(), module));
            }

            llvm::Type* ret_type = type_to_llvm_type(
                fn_type->get_return_type().get(),
                module
            );
            llvm::FunctionType* llvm_fn_type = llvm::FunctionType::get(
                ret_type,
                param_types,
                false
            );

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
                    auto* global = llvm::cast<llvm::GlobalVariable>(fn_ptr_val);
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
                const auto& arguments = this->get_arguments();
                for (size_t i = 0; i < arguments.size(); ++i)
                {
                    auto* arg_val = arguments[i]->codegen(
                        module,
                        builder);
                    if (!arg_val)
                        return nullptr;

                    if (i < param_types.size() && arg_val->getType() !=
                        param_types[i])
                    {
                        arg_val = unwrap_optional_value(arg_val, builder);
                    }
                    args_v.push_back(arg_val);
                }

                const auto instruction_name = ret_type->isVoidTy()
                    ? ""
                    : "indcalltmp";
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

std::unique_ptr<IAstNode> AstFunctionCall::clone()
{
    std::vector<std::unique_ptr<IAstExpression>> cloned_args;
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
