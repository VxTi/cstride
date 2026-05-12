#include "ast/conditionals.h"

#include "errors.h"
#include "ast/nodes/expression.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

llvm::Value* stride::ast::codegen_conditional_value(
    SymbolTable* symbol_table,
    llvm::Module* module,
    llvm::IRBuilderBase* builder,
    IAstExpression* condition
)
{
    if (!condition) // Fall back to 1 if no condition is provided, e.g., in `if { ... }`
    {
        return builder->getInt1(true);
    }

    llvm::Value* condValue = condition->codegen(symbol_table, module, builder);

    if (condValue == nullptr)
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            "Could not generate conditional value",
            condition->get_source_position()
        );
    }

    // Ensure condValue is of type i1
    if (condValue->getType()->isIntegerTy())
    {
        if (condValue->getType()->getIntegerBitWidth() != 1)
        {
            return builder->CreateICmpNE(
                condValue,
                llvm::ConstantInt::get(condValue->getType(), 0, false)
            );
        }
        return condValue;
    }

    throw stride_error(
        ErrorType::COMPILATION_ERROR,
        "Condition must be a boolean type",
        condition->get_source_position()
    );
}
