#include "ast/closures.h"
#include "ast/nodes/function_declaration.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>

using namespace stride::ast;

llvm::Value* IAstFunction::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Function* function = nullptr;

    for (const auto& [function_name, llvm_function_val] : this->get_generic_function_metadata())
    {
        if (!llvm_function_val)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Function symbol '{}' missing", function_name),
                this->get_source_fragment()
            );
        }


        // If the function body has already been generated (has basic blocks), just return the function pointer
        if (this->is_extern() || !llvm_function_val->empty())
        {
            return llvm_function_val;
        }

        // Save the current insert point to restore it later
        // This is important when generating nested lambdas
        llvm::BasicBlock* saved_insert_block = builder->GetInsertBlock();
        llvm::BasicBlock::iterator saved_insert_point;
        const bool has_insert_point = saved_insert_block != nullptr;

        if (saved_insert_block)
        {
            saved_insert_point = builder->GetInsertPoint();
        }

        llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(
            module->getContext(),
            "entry",
            llvm_function_val
        );
        builder->SetInsertPoint(entry_bb);

        // We create a new builder for the prologue to ensure allocas are at the very top
        llvm::IRBuilder prologue_builder(
            &llvm_function_val->getEntryBlock(),
            llvm_function_val->getEntryBlock().begin()
        );

        //
        // Captured variable handling
        // Map captured variables to function arguments with __capture_ prefix
        //
        auto fn_parameter_argument = llvm_function_val->arg_begin();
        for (const auto& capture : this->_captured_variables)
        {
            if (fn_parameter_argument != llvm_function_val->arg_end())
            {
                fn_parameter_argument->setName(closures::format_captured_variable_name(capture.internal_name));

                // Create alloca with __capture_ prefix so identifier lookup can find it
                llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                    fn_parameter_argument->getType(),
                    nullptr,
                    closures::format_captured_variable_name_internal(capture.internal_name)
                );

                builder->CreateStore(fn_parameter_argument, alloca);
                ++fn_parameter_argument;
            }
        }

        //
        // Function parameter handling
        // Here we define the parameters on the stack as memory slots for the function
        //
        for (const auto& param : this->_parameters)
        {
            if (fn_parameter_argument != llvm_function_val->arg_end())
            {
                fn_parameter_argument->setName(param->get_name() + ".arg");

                // Create a memory slot on the stack for the parameter
                llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                    fn_parameter_argument->getType(),
                    nullptr,
                    param->get_name()
                );

                // Store the initial argument value into the alloca
                builder->CreateStore(fn_parameter_argument, alloca);

                ++fn_parameter_argument;
            }
        }

        // Generate Body
        llvm::Value* function_body_value = this->_body->codegen(module, builder);

        // Final Safety: Implicit Return
        // If the get_body didn't explicitly return (no terminator found), add one.
        if (llvm::BasicBlock* current_bb = builder->GetInsertBlock();
            current_bb && !current_bb->getTerminator())
        {
            if (llvm::Type* ret_type = llvm_function_val->getReturnType();
                ret_type->isVoidTy())
            {
                builder->CreateRetVoid();
            }
            else if (function_body_value && function_body_value->getType() == ret_type)
            {
                builder->CreateRet(function_body_value);
            }
            // Default return to keep IR valid (useful for main or incomplete functions)
            else
            {
                if (ret_type->isFloatingPointTy())
                {
                    builder->CreateRet(llvm::ConstantFP::get(ret_type, 0.0));
                }
                else if (ret_type->isIntegerTy())
                {
                    builder->CreateRet(llvm::ConstantInt::get(ret_type, 0));
                }
                else
                {
                    throw parsing_error(
                        ErrorType::COMPILATION_ERROR,
                        std::format(
                            "Function '{}' is missing a return path.",
                            this->get_plain_function_name()
                        ),
                        this->get_source_fragment()
                    );
                }
            }
        }

        if (llvm::verifyFunction(*llvm_function_val, &llvm::errs()))
        {
            module->print(llvm::errs(), nullptr);
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "LLVM Function Verification Failed for: " + this->get_plain_function_name(),
                this->get_source_fragment()
            );
        }

        // Restore the previous insert point for nested lambda generation
        if (has_insert_point && saved_insert_block)
        {
            builder->SetInsertPoint(saved_insert_block, saved_insert_point);
        }

        // For anonymous lambdas with captured variables, create a closure structure
        // that bundles the function pointer with the current values of captured variables
        if (this->is_anonymous())
        {
            // Collect the current values of captured variables from the enclosing scope
            std::vector<llvm::Value*> captured_values;
            for (const auto& capture : this->get_captured_variables())
            {
                if (const auto block = builder->GetInsertBlock())
                {
                    llvm::Function* current_fn = block->getParent();
                    llvm::Value* captured_val = closures::lookup_variable_or_capture(current_fn, capture.internal_name);

                    if (!captured_val)
                    {
                        captured_val = closures::lookup_variable_by_base_name(current_fn, capture.name);
                    }

                    if (captured_val)
                    {
                        // Load the value if it's an alloca
                        if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(captured_val))
                        {
                            captured_val = builder->CreateLoad(
                                alloca->getAllocatedType(),
                                alloca,
                                capture.internal_name
                            );
                        }
                        captured_values.push_back(captured_val);
                    }
                }
            }

            // Create and return a closure instead of the raw function pointer
            return closures::create_closure(module, builder, llvm_function_val, captured_values);
        }

        function = llvm_function_val;
    }

    return function;
}
