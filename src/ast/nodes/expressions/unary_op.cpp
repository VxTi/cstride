#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

using namespace stride::ast;

bool requires_identifier_operand(const UnaryOpType op)
{
    switch (op)
    {
    case UnaryOpType::INCREMENT:
    case UnaryOpType::DECREMENT:
    case UnaryOpType::ADDRESS_OF:
        return true;
    default: return false;
    }
}

/**
 * Certain operations require a RHS operand, e.g., `~<num>`
 */
bool requires_rhs_operand(const UnaryOpType op)
{
    // Technically all prefix operators require a RHS operand.
    // However, if this function implies "Does this operator syntax REQUIRE an operand to follow?", then yes.
    return true;
}

std::optional<UnaryOpType> stride::ast::get_unary_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::BANG: return UnaryOpType::LOGICAL_NOT;
    case TokenType::MINUS: return UnaryOpType::NEGATE;
    case TokenType::TILDE: return UnaryOpType::COMPLEMENT;
    case TokenType::DOUBLE_PLUS: return UnaryOpType::INCREMENT;
    case TokenType::DOUBLE_MINUS: return UnaryOpType::DECREMENT;
    case TokenType::STAR: return UnaryOpType::DEREFERENCE;
    case TokenType::AMPERSAND: return UnaryOpType::ADDRESS_OF;
    default: break;
    }
    return std::nullopt;
}

llvm::Value* AstUnaryOp::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    if (requires_identifier_operand(this->get_op_type()))
    {
        // These operations require an LValue (address), effectively only working on variables (identifiers)
        const auto* identifier = dynamic_cast<AstIdentifier*>(&this->get_operand());

        if (!identifier)
        {
            throw parsing_error(
                make_ast_error(
                    *this->source,
                    this->source_offset,
                    "Operand must be an identifier for this operation"
                )
            );
        }

        const auto internal_name = identifier->get_internal_name();

        llvm::Value* var_addr = nullptr;
        if (const auto block = builder->GetInsertBlock())
        {
            if (const auto function = block->getParent())
            {
                var_addr = function->getValueSymbolTable()->lookup(internal_name);
            }
        }

        if (!var_addr)
        {
            // var_addr = module->getNamedGlobal(internal_name);

            // Try global
            module->getNamedGlobal(identifier->get_name());
        }

        if (!var_addr)
        {
            throw parsing_error(
                make_ast_error(
                    *this->source,
                    this->source_offset,
                    "Unknown variable: " + internal_name
                )
            );
        }

        // Address Of (&x) just returns the address
        if (this->get_op_type() == UnaryOpType::ADDRESS_OF)
        {
            return var_addr;
        }

        llvm::Type* loaded_type = nullptr;
        if (const auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(var_addr))
        {
            loaded_type = alloca->getAllocatedType();
        }
        else if (const auto* global = llvm::dyn_cast<llvm::GlobalVariable>(var_addr))
        {
            loaded_type = global->getValueType();
        }
        else
        {
            throw parsing_error(
                make_ast_error(
                    *this->source,
                    this->source_offset,
                    "Cannot determine type of variable: " + internal_name
                )
            );
        }

        // Increment / Decrement
        auto* loaded_val = builder->CreateLoad(loaded_type, var_addr, "loadtmp");
        llvm::Value* one = llvm::ConstantInt::get(loaded_val->getType(), 1);
        llvm::Value* new_val;

        if (this->get_op_type() == UnaryOpType::INCREMENT)
        {
            new_val = builder->CreateAdd(loaded_val, one, "inctmp");
        }
        else
        {
            new_val = builder->CreateSub(loaded_val, one, "dectmp");
        }

        builder->CreateStore(new_val, var_addr);

        // Postfix returns old value, Prefix returns new value
        return this->is_lsh() ? loaded_val : new_val;
    }

    auto* val = get_operand().codegen(module, context, builder);
    if (!val) return nullptr;

    switch (this->get_op_type())
    {
    case UnaryOpType::LOGICAL_NOT:
        {
            // !x equivalent to (x == 0)
            auto* cmp = builder->CreateICmpEQ(val, llvm::ConstantInt::get(val->getType(), 0), "lognotcmp");
            return builder->CreateZExt(cmp, val->getType(), "lognot");
        }
    case UnaryOpType::NEGATE:
        return builder->CreateNeg(val, "neg");
    case UnaryOpType::COMPLEMENT:
        return builder->CreateNot(val, "not");
    case UnaryOpType::DEREFERENCE:
        // Requires type system to know what we are pointing to
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Dereference not implemented yet due to opaque pointers")
        );
    default:
        return nullptr;
    }
}

std::optional<std::unique_ptr<AstExpression>> stride::ast::parse_binary_unary_op(const Scope& scope, TokenSet& set)
{
    const auto next = set.peak_next();

    // Prefix Parsing
    if (const auto op_type = get_unary_op_type(next.type); op_type.has_value())
    {
        set.next(); // Consume operator

        // Recursive call to handle chained unaries like !!x or - -x etc.
        // We call parse_standalone_expression_part, which will call parse_binary_unary_op again.
        auto distinct_expr = parse_standalone_expression_part(scope, set);
        if (!distinct_expr)
        {
            set.throw_error("Expected expression after unary operator");
        }

        // Validation for identifiers
        if (requires_identifier_operand(op_type.value()))
        {
            if (!dynamic_cast<AstIdentifier*>(distinct_expr.get()))
            {
                set.throw_error("Unary operator requires identifier operand");
            }
        }

        return std::make_unique<AstUnaryOp>(
            set.source(),
            next.offset,
            op_type.value(),
            std::move(distinct_expr),
            false // Prefix
        );
    }

    // Check for Postfix Unary (only Identifier supported for provided context/examples)
    // We can't easily hijack arbitrary expression parsing for postfix here without restructuring `parse_standalone_expression_part`.
    // However, we can peek if we have Identifier -> PostfixOp
    if (set.peak_next_eq(TokenType::IDENTIFIER) && (
        // Check if the token AFTER identifier is ++ or --
        set.peak(1).type == TokenType::DOUBLE_PLUS || set.peak(1).type == TokenType::DOUBLE_MINUS))
    {
        const auto iden_tok = set.next();
        const auto iden_name = iden_tok.lexeme;

        const auto operation_tok = set.next();

        const auto internal_name = scope.get_symbol_globally(iden_name)
                                        .transform([](const SymbolDefinition& def) { return def.get_internal_name(); })
                                        .value_or(iden_tok.lexeme);

        UnaryOpType type = (operation_tok.type == TokenType::DOUBLE_PLUS)
                               ? UnaryOpType::INCREMENT
                               : UnaryOpType::DECREMENT;

        return std::make_unique<AstUnaryOp>(
            set.source(),
            iden_tok.offset,
            type,
            std::make_unique<AstIdentifier>(
                set.source(),
                next.offset,
                iden_name,
                internal_name
            ),
            true // Postfix
        );
    }

    return std::nullopt;
}
