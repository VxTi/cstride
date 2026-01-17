#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

bool stride::ast::is_variable_declaration(const TokenSet& set)
{
    // let k: i8 = 123
    return (
        set.peak_eq(TokenType::KEYWORD_LET, 0) &&
        set.peak_eq(TokenType::IDENTIFIER, 1) &&
        set.peak_eq(TokenType::COLON, 2) &&
        (
            // Type can be either a primitive or a user-defined identifier
            set.peak_eq(TokenType::IDENTIFIER, 3) || is_primitive(set.peak(3).type)
        )
        && set.peak_eq(TokenType::EQUALS, 4)
    );
}

llvm::Value* AstVariableDeclaration::codegen(llvm::Module* module, llvm::LLVMContext& context,
                                             llvm::IRBuilder<>* irbuilder)
{
    // Generate code for the initial value
    llvm::Value* init_value = nullptr;
    if (this->initial_value != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->initial_value.get()))
        {
            init_value = synthesisable->codegen(module, context, irbuilder);
        }
    }

    // Get the LLVM type for the variable
    llvm::Type* var_type = types::internal_type_to_llvm_type(this->variable_type.get(), module, context);

    // Check if we have a valid insert block
    llvm::BasicBlock* current_block = irbuilder->GetInsertBlock();
    if (current_block == nullptr)
    {
        return nullptr;
    }

    // Find the entry block of the current function
    llvm::Function* current_function = current_block->getParent();
    if (current_function == nullptr)
    {
        return nullptr;
    }

    llvm::BasicBlock& entry_block = current_function->getEntryBlock();
    llvm::IRBuilder<>::InsertPoint save_point = irbuilder->saveIP();
    irbuilder->SetInsertPoint(&entry_block, entry_block.begin());

    llvm::AllocaInst* alloca = irbuilder->CreateAlloca(var_type, nullptr, this->variable_name.value);

    // Restore the insert point
    irbuilder->restoreIP(save_point);

    // Store the initial value if it exists
    if (init_value != nullptr)
    {
        irbuilder->CreateStore(init_value, alloca);
    }

    return alloca;
}

std::string AstVariableDeclaration::to_string()
{
    return std::format(
        "VariableDeclaration({}, {}, {})",
        variable_name.value,
        variable_type->to_string(),
        initial_value ? initial_value->to_string() : "nil"
    );
}