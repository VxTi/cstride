#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

/**
 * Checks whether the provided sequence conforms to
 * "let name: type = value",
 * or "extern let name: type;"
 */
bool stride::ast::is_variable_declaration(const TokenSet& set)
{
    // External variables cannot have initializers
    if (set.peak_next_eq(TokenType::KEYWORD_EXTERN))
    {
        return set.peak_eq(TokenType::KEYWORD_LET, 1) &&
            set.peak_eq(TokenType::IDENTIFIER, 2) &&
            set.peak_eq(TokenType::COLON, 3) &&
            (
                // Type can be either a primitive or a user-defined identifier
                set.peak_eq(TokenType::IDENTIFIER, 4) || is_primitive_type(set.peak(4).type)
            )
            && set.peak_eq(TokenType::SEMICOLON, 5);
    }

    return (
        (set.peak_eq(TokenType::KEYWORD_LET, 0) || set.peak_eq(TokenType::KEYWORD_MUT, 0)) &&
        set.peak_eq(TokenType::IDENTIFIER, 1) &&
        set.peak_eq(TokenType::COLON, 2) &&
        (
            // Type can be either a primitive or a user-defined identifier
            set.peak_eq(TokenType::IDENTIFIER, 3) || is_primitive_type(set.peak(3).type)
        )
        && set.peak_eq(TokenType::EQUALS, 4)
    );
}

llvm::Value* AstVariableDeclaration::codegen(
    const std::shared_ptr<Scope>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context, llvm::IRBuilder<>* irBuilder
)
{
    // Generate code for the initial value
    llvm::Value* init_value = nullptr;
    if (const auto initial_value = this->get_initial_value().get(); initial_value != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(initial_value))
        {
            init_value = synthesisable->codegen(scope, module, context, irBuilder);
        }
    }

    // Get the LLVM type for the variable
    llvm::Type* var_type = internal_type_to_llvm_type(this->get_variable_type(), module, context);

    // If this is a pointer type, convert to pointer
    if (this->get_variable_type()->is_pointer())
    {
        var_type = llvm::PointerType::get(context, 0);
    }

    // Check if this is a global variable declaration
    if ((this->get_flags() & SRFLAG_VAR_DECL_GLOBAL) != 0)
    {
        // Create a global variable
        llvm::Constant* initializer = nullptr;

        if (init_value != nullptr)
        {
            if (auto* constant = llvm::dyn_cast<llvm::Constant>(init_value))
            {
                initializer = constant;
            }
            else
            {
                // Global variables require constant initializers
                initializer = llvm::Constant::getNullValue(var_type);
            }
        }
        else
        {
            initializer = llvm::Constant::getNullValue(var_type);
        }

        return new llvm::GlobalVariable(
            *module,
            var_type,
            (this->get_flags() & SRFLAG_VAR_DECL_MUTABLE) == 0, // isConstant
            llvm::GlobalValue::ExternalLinkage,
            initializer,
            this->get_internal_name()
        );
    }

    // Local variable - use AllocaInst
    // Check if we have a valid insert block
    llvm::BasicBlock* current_block = irBuilder->GetInsertBlock();
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
    const llvm::IRBuilder<>::InsertPoint save_point = irBuilder->saveIP();
    irBuilder->SetInsertPoint(&entry_block, entry_block.begin());

    llvm::AllocaInst* alloca = irBuilder->CreateAlloca(var_type, nullptr, this->get_internal_name());

    // Restore the insert point
    irBuilder->restoreIP(save_point);

    // Store the initial value if it exists
    if (init_value != nullptr)
    {
        if (init_value->getType() != var_type && init_value->getType()->isIntegerTy() && var_type->isIntegerTy())
        {
            init_value = irBuilder->CreateIntCast(init_value, var_type, true);
        }

        irBuilder->CreateStore(init_value, alloca);
    }

    return alloca;
}

void AstVariableDeclaration::validate()
{
    const auto internal_expr_type = infer_expression_type(scope, this->get_initial_value().get());

    if (IAstInternalFieldType* rhs_type = internal_expr_type.get(); *this->get_variable_type() != *rhs_type)
    {
        throw parsing_error(
            make_ast_error(
                *source,
                source_offset,
                std::format(
                    "Type mismatch in variable declaration; expected type '{}', got '{}'",
                    this->get_variable_type()->to_string(),
                    internal_expr_type->to_string()
                )
            )
        );
    }
}


bool AstVariableDeclaration::is_reducible()
{
    if (this->get_initial_value() == nullptr)
    {
        return false;
    }
    // Variables are reducible only if their initial value is reducible,
    // In the future we can also check whether variables are ever refereced,
    // in which case we can optimize away the variable declaration.
    if (const auto value = dynamic_cast<IReducible*>(this->get_initial_value().get()))
    {
        return value->is_reducible();
    }

    return false;
}

IAstNode* AstVariableDeclaration::reduce()
{
    if (this->is_reducible())
    {
        const auto reduced_value = dynamic_cast<IReducible*>(this->get_initial_value().get())->reduce();
        auto cloned_type = this->get_variable_type()->clone();

        if (auto* reduced_expr = dynamic_cast<AstExpression*>(reduced_value); reduced_expr != nullptr)
        {
            return std::make_unique<AstVariableDeclaration>(
                this->source,
                this->source_offset,
                scope,
                this->get_variable_name(),
                std::move(cloned_type),
                std::unique_ptr<AstExpression>(reduced_expr),
                this->get_flags(),
                this->get_internal_name()
            ).release();
        }
    }
    return this;
}

std::unique_ptr<AstVariableDeclaration> stride::ast::parse_variable_declaration(
    const int expression_type_flags,
    const std::shared_ptr<Scope>& scope,
    TokenSet& set
)
{
    if ((expression_type_flags & SRFLAG_EXPR_ALLOW_VARIABLE_DECLARATION) == 0)
    {
        set.throw_error("Variable declarations are not allowed in this context");
    }

    int flags = 0;

    if (scope->get_scope_type() == ScopeType::GLOBAL)
    {
        flags |= SRFLAG_VAR_DECL_GLOBAL;
    }

    auto reference_token = set.peak_next();

    if (set.peak_next_eq(TokenType::KEYWORD_MUT))
    {
        flags |= SRFLAG_VAR_DECL_MUTABLE;
        set.next();
    }
    else
    {
        // Variables are const by default.
        set.expect(TokenType::KEYWORD_LET);
    }

    const auto variable_name_tok = set.expect(TokenType::IDENTIFIER, "Expected variable name in variable declaration");
    const auto variable_name = variable_name_tok.lexeme;
    set.expect(TokenType::COLON);
    auto variable_type = parse_ast_type(scope, set);

    set.expect(TokenType::EQUALS);

    auto value = parse_expression_ext(
        SRFLAG_EXPR_VARIABLE_ASSIGNATION,
        scope, set
    );

    // If it's not an inline variable declaration (e.g., in a for loop),
    // we expect a semicolon at the end.
    if ((expression_type_flags & SRFLAG_EXPR_INLINE_VARIABLE_DECLARATION) == 0)
    {
        set.expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    }

    std::string internal_name = variable_name;
    if (scope->get_scope_type() != ScopeType::GLOBAL)
    {
        static int var_decl_counter = 0;
        internal_name = std::format("{}.{}", variable_name, var_decl_counter++);
    }

    scope->define_field(
        variable_name,
        internal_name,
        variable_type.get(),
        flags
    );

    return std::make_unique<AstVariableDeclaration>(
        set.source(),
        reference_token.offset,
        scope,
        variable_name,
        std::move(variable_type),
        std::move(value),
        flags,
        internal_name
    );
}

std::string AstVariableDeclaration::to_string()
{
    return std::format(
        "VariableDeclaration({}({}), {}, {})",
        get_variable_name(),
        get_internal_name(),
        get_variable_type()->to_string(),
        this->get_initial_value() ? get_initial_value()->to_string() : "nil"
    );
}
