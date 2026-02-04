#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "ast/flags.h"
#include "ast/optionals.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::unique_ptr<AstVariableDeclaration> stride::ast::parse_variable_declaration(
    const int expression_type_flags,
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    // Ensure we're allowed to parse standalone expressions
    if ((expression_type_flags & SRFLAG_EXPR_TYPE_STANDALONE) == 0)
    {
        set.throw_error("Variable declarations are not allowed in this context");
    }

    int flags = 0;

    if (context->get_current_scope_type() == ScopeType::GLOBAL)
    {
        flags |= SRFLAG_TYPE_GLOBAL;
    }

    const auto reference_token = set.peek_next();

    if (set.peek_next_eq(TokenType::KEYWORD_LET))
    {
        flags |= SRFLAG_TYPE_MUTABLE;
        set.next();
    }
    else
    {
        // Variables are const by default.
        set.expect(TokenType::KEYWORD_CONST);
    }

    const auto variable_name_tok = set.expect(TokenType::IDENTIFIER, "Expected variable name in variable declaration");
    const auto& variable_name = variable_name_tok.get_lexeme();
    set.expect(TokenType::COLON);
    auto variable_type = parse_type(context, set, "Expected variable type after variable name", flags);


    std::unique_ptr<AstExpression> value = nullptr;

    if (set.peek_next_eq(TokenType::EQUALS))
    {
        set.next();
        value = parse_inline_expression(context, set);
    }
    else
    {
        if (!variable_type->is_optional())
        {
            throw parsing_error(
                ErrorType::SYNTAX_ERROR,
                "Expected '=' after non-optional variable declaration",
                *variable_type->get_source(),
                variable_type->get_source_position()
            );
        }

        const auto ref_src_pos = reference_token.get_source_position();
        const auto var_type_src_pos = variable_type->get_source_position();

        // If no expression was provided (lacking '='), initialize with nil if the initial type is optional
        value = std::make_unique<AstNilLiteral>(
            set.get_source(),
            SourcePosition(
                ref_src_pos.offset,
                var_type_src_pos.offset + var_type_src_pos.length - ref_src_pos.offset
            ),
            context
        );
    }


    std::string internal_name = variable_name;
    if (context->get_current_scope_type() != ScopeType::GLOBAL)
    {
        static int var_decl_counter = 0;
        internal_name = std::format("{}.{}", variable_name, var_decl_counter++);
    }

    context->define_variable(
        variable_name,
        internal_name,
        variable_type->clone()
    );

    const auto ref_tok_pos = reference_token.get_source_position();
    const auto var_type_pos = variable_type->get_source_position();

    return std::make_unique<AstVariableDeclaration>(
        set.get_source(),
        SourcePosition(ref_tok_pos.offset, var_type_pos.offset + var_type_pos.length - ref_tok_pos.offset),
        context,
        variable_name,
        internal_name,
        std::move(variable_type),
        std::move(value)
    );
}

/**
 * Checks whether the provided sequence conforms to
 * "let name: type = value",
 * or "extern let name: type;"
 */
bool stride::ast::is_variable_declaration(const TokenSet& set)
{
    const int offset = set.peek_next_eq(TokenType::KEYWORD_EXTERN) ? 1 : 0; // Offset the initial token

    // This assumes one of the following sequences is a "variable declaration":
    // extern k:
    // mut k:
    // let k:
    return (
        (set.peek_eq(TokenType::KEYWORD_CONST, offset) || set.peek_eq(TokenType::KEYWORD_LET, offset)) &&
        set.peek_eq(TokenType::IDENTIFIER, offset + 1) &&
        set.peek_eq(TokenType::COLON, offset + 2)
    );
}

void AstVariableDeclaration::validate()
{
    AstExpression* init_val = this->get_initial_value().get();
    if (!init_val)
    {
        // It's possible that there's no initializer value.
        // No need to further validate then
        return;
    }

    init_val->validate();

    const std::unique_ptr<IAstType> internal_expr_type = infer_expression_type(
        this->get_registry(),
        init_val
    );
    const auto lhs_type = this->get_variable_type();

    if (const auto rhs_type = internal_expr_type.get();
        !lhs_type->equals(*rhs_type))
    {
        if (const auto primitive_type = dynamic_cast<AstPrimitiveType*>(rhs_type);
            primitive_type != nullptr &&
            primitive_type->get_type() == PrimitiveType::NIL)
        {
            if (lhs_type->get_flags() & SRFLAG_TYPE_OPTIONAL) return;

            const std::vector references = {
                ErrorSourceReference(
                    lhs_type->to_string(),
                    *this->get_source(),
                    this->get_source_position()
                ),
                ErrorSourceReference(
                    rhs_type->to_string(),
                    *this->get_source(),
                    init_val->get_source_position()
                )
            };

            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Cannot assign nil to variable of non-optional type '{}'",
                    lhs_type->to_string()
                ),
                references
            );
        }

        const std::string lhs_type_str = lhs_type->to_string();
        const std::string rhs_type_str = rhs_type->to_string();

        const std::vector references = {
            ErrorSourceReference(
                lhs_type_str,
                *this->get_source(),
                this->get_source_position()
            ),
            ErrorSourceReference(
                rhs_type_str,
                *this->get_source(),
                init_val->get_source_position()
            )
        };

        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Type mismatch in variable declaration; expected type '{}', got '{}'",
                lhs_type_str,
                rhs_type_str
            ),
            references
        );
    }
}

// LLVM calls these functions at startup to initialize global variables
// This way, we can assign function return values to variables
void append_to_global_ctors(llvm::Module* module, llvm::Function* init_func, const int priority)
{
    llvm::IRBuilder<> ib(module->getContext());

    // The struct type: { i32, void ()*, i8* }
    llvm::StructType* ctor_struct_type = llvm::StructType::get(
        ib.getInt32Ty(),
        init_func->getType(),
        ib.getPtrTy()
    );

    // Create the initializer entry
    llvm::Constant* entry = llvm::ConstantStruct::get(
        ctor_struct_type, {
            ib.getInt32(priority),
            init_func,
            llvm::ConstantPointerNull::get(ib.getPtrTy())
        });

    // Get existing ctors or create new ones
    llvm::GlobalVariable* existing_ctors = module->getGlobalVariable("llvm.global_ctors");
    std::vector<llvm::Constant*> ctor_list;

    if (existing_ctors)
    {
        if (auto* initializer = llvm::dyn_cast<llvm::ConstantArray>(existing_ctors->getInitializer()))
        {
            for (unsigned i = 0; i < initializer->getNumOperands(); ++i)
            {
                ctor_list.push_back(initializer->getOperand(i));
            }
        }
        existing_ctors->eraseFromParent();
    }

    ctor_list.push_back(entry);

    // Create the new array
    llvm::ArrayType* array_type = llvm::ArrayType::get(ctor_struct_type, ctor_list.size());
    new llvm::GlobalVariable(
        *module,
        array_type,
        /* isConstant = */ false,
        llvm::GlobalValue::AppendingLinkage,
        llvm::ConstantArray::get(array_type, ctor_list),
        "llvm.global_ctors"
    );
}

void AstVariableDeclaration::resolve_forward_references(
    const std::shared_ptr<ParsingContext>& context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    if (!this->get_variable_type()->is_global())
    {
        return;
    }

    llvm::Type* var_type = internal_type_to_llvm_type(this->get_variable_type(), module);
    if (!var_type) return;

    // Check if it already exists (should not happen, but for safety)
    if (module->getNamedGlobal(this->get_internal_name())) return;

    // Create the Global Variable with a default null initializer
    llvm::Constant* default_init = llvm::Constant::getNullValue(var_type);

    new llvm::GlobalVariable(
        *module,
        var_type,
        !this->get_variable_type()->is_mutable(),
        llvm::GlobalValue::ExternalLinkage,
        default_init,
        this->get_internal_name()
    );
}

void global_var_declaration_codegen(
    const AstVariableDeclaration* self,
    llvm::GlobalVariable* global_var,
    llvm::Module* module,
    llvm::IRBuilder<>* irBuilder
)
{
    // Dynamic Initialization ("Global Constructor" Pattern)
    // Create a function: void __init_variable_name()
    const std::string func_name = "__init_global_" + self->get_internal_name();
    llvm::FunctionType* func_type = llvm::FunctionType::get(irBuilder->getVoidTy(), false);
    llvm::Function* init_func = llvm::Function::Create(
        func_type, llvm::GlobalValue::InternalLinkage, func_name, module
    );

    // Set up the entry block for the constructor
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(module->getContext(), "entry", init_func);

    // Create a temporary builder for the constructor to avoid state pollution
    llvm::IRBuilder<> tempBuilder(module->getContext());
    tempBuilder.SetInsertPoint(entry);

    // Re-generate the initial value inside the constructor function
    // Note: we MUST NOT call codegen on something that might have already been generated
    // if it's not designed to be called multiple times.
    // However, for global initialization, we need the value.
    llvm::Value* dynamic_init_value = nullptr;
    if (const auto initial_value = self->get_initial_value().get(); initial_value != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(initial_value))
        {
            // Use the temporary builder
            dynamic_init_value = synthesisable->codegen(self->get_registry(), module, &tempBuilder);
        }
    }

    if (dynamic_init_value)
    {
        // Perform the store
        tempBuilder.CreateStore(dynamic_init_value, global_var);
    }

    tempBuilder.CreateRetVoid();

    // Register this function in llvm.global_ctors
    append_to_global_ctors(module, init_func, 65535);
}

std::optional<llvm::GlobalVariable*> get_global_var_decl(
    const AstVariableDeclaration* self,
    llvm::Module* module,
    llvm::Type* var_type
)
{
    if (!self->get_variable_type()->is_global()) return std::nullopt;

    llvm::GlobalVariable* global_var = module->getNamedGlobal(self->get_internal_name());

    if (!global_var)
    {
        // If it wasn't created in resolve_forward_references (e.g. if we are not using it)
        // though it should have been.
        llvm::Constant* default_init = llvm::Constant::getNullValue(var_type);

        return new llvm::GlobalVariable(
            *module,
            var_type,
            false, // Set to false to allow initialization
            llvm::GlobalValue::ExternalLinkage,
            default_init,
            self->get_internal_name()
        );
    }

    // Ensure it's not constant so we can store to it in the constructor
    global_var->setConstant(false);
    return global_var;
}

llvm::Value* AstVariableDeclaration::codegen(
    const std::shared_ptr<ParsingContext>& context,
    llvm::Module* module,
    llvm::IRBuilder<>* ir_builder
)
{
    // Get the LLVM type for the variable
    llvm::Type* variable_ty = internal_type_to_llvm_type(this->get_variable_type(), module);

    // If we are generating a global, we might rely on constant initialization
    // or dynamic initialization handled later.
    if (const std::optional<llvm::GlobalVariable*> global_var = get_global_var_decl(this, module, variable_ty);
        global_var.has_value())
    {
        llvm::Value* init_value = nullptr;
        // Generate init value code only if it's a literal/constant,
        // otherwise dynamic init handles it (see global_var_declaration_codegen)
        if (const auto initial_value = this->get_initial_value().get(); initial_value != nullptr)
        {
            if (is_literal_ast_node(initial_value) && dynamic_cast<ISynthesisable*>(initial_value))
            {
                init_value = dynamic_cast<ISynthesisable*>(initial_value)->codegen(
                    this->get_registry(),
                    module,
                    ir_builder
                );
            }
        }

        if (init_value != nullptr)
        {
            if (auto* constant = llvm::dyn_cast<llvm::Constant>(init_value))
            {
                global_var.value()->setInitializer(constant);
            }
        }
        else
        {
            global_var_declaration_codegen(this, global_var.value(), module, ir_builder);
        }
        return global_var.value();
    }

    // Create the Alloca in the Entry block to ensure it dominates all uses
    // and name it so Identifier lookups can find it.
    llvm::Function* function = ir_builder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> entry_builder(&function->getEntryBlock(), function->getEntryBlock().begin());

    llvm::AllocaInst* alloca = entry_builder.CreateAlloca(
        variable_ty,
        nullptr,
        this->get_internal_name() // This registers it in the SymbolTable
    );

    // Generate code for the initial value at the current insertion point
    llvm::Value* init_value = nullptr;
    if (const auto initial_value = this->get_initial_value().get(); initial_value != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(initial_value))
        {
            init_value = synthesisable->codegen(this->get_registry(), module, ir_builder);
        }
    }

    if (init_value)
    {
        llvm::Value* value_to_store = nullptr;

        // Handle Optional Wrapping: Value T -> Optional<T>
        if (is_optional_wrapped_type(variable_ty))
        {
            value_to_store = wrap_optional_value(init_value, variable_ty, ir_builder);
        }
        else
        {
            // Handle basic upcasts (e.g. i32 -> i64, etc)
            value_to_store = optionally_upcast_type(init_value, variable_ty, ir_builder);
        }

        ir_builder->CreateStore(value_to_store, alloca);
    }

    return alloca;
}

bool AstVariableDeclaration::is_reducible()
{
    if (this->get_initial_value() == nullptr)
    {
        return false;
    }
    // Variables are reducible only if their initial value is reducible,
    // In the future we can also check whether variables are ever referenced,
    // in which case we can optimize away the variable declaration.

    return this->get_initial_value()->is_reducible();
}

IAstNode* AstVariableDeclaration::reduce()
{
    const auto reduced_value = dynamic_cast<IReducible*>(this->get_initial_value().get())->reduce();
    auto cloned_type = this->get_variable_type()->clone();

    if (auto* reduced_expr = dynamic_cast<AstExpression*>(reduced_value); reduced_expr != nullptr)
    {
        return std::make_unique<AstVariableDeclaration>(
            this->get_source(),
            this->get_source_position(),
            this->get_registry(),
            this->get_variable_name(),
            this->get_internal_name(),
            std::move(cloned_type),
            std::unique_ptr<AstExpression>(reduced_expr)
        ).release();
    }
    return this;
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
