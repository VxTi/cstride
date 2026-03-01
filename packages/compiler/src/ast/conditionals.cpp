#include "ast/conditionals.h"

#include "errors.h"
#include "ast/nodes/expression.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

llvm::Value* stride::ast::codegen_conditional_value(
    llvm::Module* module,
    llvm::IRBuilderBase* builder,
    IAstExpression* condition
)
{
    if (!condition) // Fall back to 1 if no condition is provided, e.g., in `if { ... }`
    {
        return llvm::ConstantInt::get(module->getContext(), llvm::APInt(1, 1));
    }

    llvm::Value* condValue = condition->codegen(module, builder);

    if (condValue == nullptr)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Could not generate conditional value",
            condition->get_source_fragment()
        );
    }

    // Ensure condValue is of type i1
    if (condValue->getType()->isIntegerTy())
    {
        if (condValue->getType()->getIntegerBitWidth() != 1)
        {
            return builder->CreateICmpNE(
                condValue,
                llvm::ConstantInt::get(module->getContext(), llvm::APInt(1, 0))
            );
        }
        return condValue;
    }

    throw parsing_error(
        ErrorType::COMPILATION_ERROR,
        "Condition must be a boolean type",
        condition->get_source_fragment()
    );
}
