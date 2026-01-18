#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

bool stride::ast::is_variable_declaration(const TokenSet& set)
{
    // let k: i8 = 123
    int offset = set.peak_next_eq(TokenType::KEYWORD_EXTERN) ? 1 : 0;
    return (
        set.peak_eq(TokenType::KEYWORD_LET, 0 + offset) &&
        set.peak_eq(TokenType::IDENTIFIER, 1 + offset) &&
        set.peak_eq(TokenType::COLON, 2 + offset) &&
        (
            // Type can be either a primitive or a user-defined identifier
            set.peak_eq(TokenType::IDENTIFIER, 3 + offset) || is_primitive(set.peak(3 + offset).type)
        )
        && set.peak_eq(TokenType::EQUALS, 4 + offset)
    );
}

llvm::Value* AstVariableDeclaration::codegen(llvm::Module* module, llvm::LLVMContext& context,
                                             llvm::IRBuilder<>* irbuilder)
{
    // Generate code for the initial value
    llvm::Value* init_value = nullptr;
    if (const auto initial_value = this->get_initial_value().get(); initial_value != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(initial_value))
        {
            init_value = synthesisable->codegen(module, context, irbuilder);
        }
    }

    // Get the LLVM type for the variable
    llvm::Type* var_type = types::internal_type_to_llvm_type(this->get_variable_type(), module, context);

    // If this is a pointer type, convert to pointer
    if (this->get_variable_type()->is_pointer())
    {
        var_type = llvm::PointerType::get(context, 0);
    }

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
    const llvm::IRBuilder<>::InsertPoint save_point = irbuilder->saveIP();
    irbuilder->SetInsertPoint(&entry_block, entry_block.begin());

    llvm::AllocaInst* alloca = irbuilder->CreateAlloca(var_type, nullptr, this->get_variable_name().value);

    // Restore the insert point
    irbuilder->restoreIP(save_point);

    // Store the initial value if it exists
    if (init_value != nullptr)
    {
        irbuilder->CreateStore(init_value, alloca);
    }

    return alloca;
}

std::unique_ptr<AstVariableDeclaration> parse_variable_declaration(
    const int expression_type_flags,
    const Scope& scope,
    TokenSet& set
)
{
    if ((expression_type_flags & EXPRESSION_ALLOW_VARIABLE_DECLARATION) == 0)
    {
        set.throw_error("Variable declarations are not allowed in this context");
    }

    int flags = 0;

    if (scope.type == ScopeType::GLOBAL)
    {
        flags |= VARIABLE_DECLARATION_FLAG_GLOBAL;
    }

    if (set.peak_next_eq(TokenType::KEYWORD_CONST))
    {
        // Variables are const by default.
        set.next();
    }
    else
    {
        flags |= VARIABLE_DECLARATION_FLAG_MUTABLE;
        set.expect(TokenType::KEYWORD_LET);
    }

    const auto variable_name_tok = set.expect(TokenType::IDENTIFIER, "Expected variable name in variable declaration");
    Symbol variable_name(variable_name_tok.lexeme);
    set.expect(TokenType::COLON);
    auto type = types::parse_primitive_type(set);

    set.expect(TokenType::EQUALS);

    auto value = parse_expression_ext(
        EXPRESSION_VARIABLE_ASSIGNATION,
        scope, set
    );

    // If it's not an inline variable declaration (e.g., in a for loop),
    // we expect a semicolon at the end.
    if ((expression_type_flags & EXPRESSION_INLINE_VARIABLE_DECLARATION) == 0)
    {
        set.expect(TokenType::SEMICOLON);
    }

    scope.try_define_scoped_symbol(*set.source(), variable_name_tok, variable_name);

    return std::make_unique<AstVariableDeclaration>(
        variable_name,
        std::move(type),
        std::move(value),
        flags
    );
}

std::string AstVariableDeclaration::to_string()
{
    return std::format(
        "VariableDeclaration({}, {}, {})",
        get_variable_name().value,
        get_variable_type()->to_string(),
        this->get_initial_value() ? get_initial_value()->to_string() : "nil"
    );
}
