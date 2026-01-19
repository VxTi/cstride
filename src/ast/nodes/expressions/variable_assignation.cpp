#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

bool AstVariableAssignment::is_reducible()
{
    if (auto* reducible = dynamic_cast<IReducible*>(this->get_value()); reducible != nullptr)
    {
        return reducible->is_reducible();
    }

    return false;
}

IAstNode* AstVariableAssignment::reduce()
{
    if (auto* reducible = dynamic_cast<IReducible*>(this->get_value()); reducible != nullptr)
    {
        return new AstVariableAssignment(
            this->source,
            this->source_offset,
            this->get_variable_name(),
            u_ptr<IAstNode>(reducible->reduce())
        );
    }

    return this;
}

llvm::Value* AstVariableAssignment::codegen(
    llvm::Module* module, llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    // Get the variable allocation
    llvm::Value* variable = module->getNamedGlobal(this->get_variable_name());
    if (!variable)
    {
        // Try to find in local scope (function)
        variable = builder->GetInsertBlock()->getValueSymbolTable()->lookup(this->get_variable_name());
    }

    if (!variable)
    {
        throw std::runtime_error("Variable '" + this->get_variable_name() + "' not found: ");
    }

    if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->get_value()); synthesisable != nullptr)
    {
        llvm::Value* value = synthesisable->codegen(module, context, builder);

        // Store the value in the variable
        builder->CreateStore(value, variable);
        return value;
    }

    return nullptr;
}

std::string AstVariableAssignment::to_string()
{
    return std::format("VariableAssignment({}, {})", this->get_variable_name(), this->get_value()->to_string());
}

bool stride::ast::is_variable_assignment(const TokenSet& set)
{
    return (
        set.peak_eq(TokenType::IDENTIFIER, 0) &&
        set.peak_eq(TokenType::EQUALS, 1) &&
        (set.peak_eq(TokenType::IDENTIFIER, 2) || is_primitive_type(set.peak(2).type))
    );
}
