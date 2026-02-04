#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

using namespace stride::ast;

std::string unary_op_type_to_str(const UnaryOpType type)
{
    switch (type)
    {
    case UnaryOpType::LOGICAL_NOT: return "!";
    case UnaryOpType::NEGATE: return "-";
    case UnaryOpType::COMPLEMENT: return "~";
    case UnaryOpType::INCREMENT: return "++";
    case UnaryOpType::DECREMENT: return "--";
    case UnaryOpType::ADDRESS_OF: return "&";
    case UnaryOpType::DEREFERENCE: return "*";

    default: return "";
    }
}

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

void AstUnaryOp::validate()
{
    const auto operand_type = infer_expression_type(this->get_registry(), this->_operand.get());

    if (!operand_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot infer type of operand",
            *this->get_source(),
            this->get_source_position()
        );
    }

    if (!operand_type->is_mutable())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot modify immutable value",
            *this->get_source(),
            this->get_source_position()
        );
    }
}

llvm::Value* AstUnaryOp::codegen(
    const std::shared_ptr<ParsingContext>& context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    if (requires_identifier_operand(this->get_op_type()))
    {
        // These operations require an LValue (address), effectively only working on variables (identifiers)
        const auto* identifier = dynamic_cast<AstIdentifier*>(&this->get_operand());

        if (!identifier)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                "Operand must be an identifier for this operation",
                *this->get_source(),
                this->get_source_position()
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
            // Try global
            var_addr = module->getNamedGlobal(identifier->get_name());
        }

        // Welp, that's it I guess
        if (!var_addr)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Unknown variable '{}'", internal_name),
                *this->get_source(),
                this->get_source_position()
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
                ErrorType::RUNTIME_ERROR,
                std::format("Cannot determine type of variable '{}'", internal_name),
                *this->get_source(),
                this->get_source_position()
            );
        }

        // Increment / Decrement
        auto* loaded_val = builder->CreateLoad(loaded_type, var_addr, "loadtmp");
        const bool is_fp = loaded_type->isFloatingPointTy();

        llvm::Value* one =
            is_fp
                ? llvm::ConstantFP::get(loaded_type, 1.0)
                : llvm::ConstantInt::get(loaded_val->getType(), 1);

        llvm::Value* new_val;

        if (this->get_op_type() == UnaryOpType::INCREMENT)
        {
            new_val =
                is_fp
                    ? builder->CreateFAdd(loaded_val, one, "inctmp")
                    : builder->CreateAdd(loaded_val, one, "inctmp");
        }
        else
        {
            new_val =
                is_fp
                    ? builder->CreateFSub(loaded_val, one, "dectmp")
                    : builder->CreateSub(loaded_val, one, "dectmp");
        }

        builder->CreateStore(new_val, var_addr);

        // Postfix returns old value, Prefix returns new value
        return this->is_lsh() ? loaded_val : new_val;
    }

    auto* val = get_operand().codegen(context, module, builder);

    if (!val) return nullptr;

    switch (this->get_op_type())
    {
    case UnaryOpType::LOGICAL_NOT:
        {
            // !x equivalent to (x == 0)
            auto* cmp = builder->CreateICmpEQ(
                val,
                llvm::ConstantInt::get(val->getType(), 0),
                "lognotcmp"
            );

            return builder->CreateZExt(cmp, val->getType(), "lognot");
        }
    case UnaryOpType::NEGATE:
        return builder->CreateNeg(val, "neg");
    case UnaryOpType::COMPLEMENT:
        return builder->CreateNot(val, "not");
    case UnaryOpType::DEREFERENCE:
        // Requires type system to know what we are pointing to
        throw parsing_error(
            ErrorType::RUNTIME_ERROR,
            "Dereference not implemented yet due to opaque pointers",
            *this->get_source(),
            this->get_source_position()
        );
    default:
        return nullptr;
    }
}

std::optional<std::unique_ptr<AstExpression>> stride::ast::parse_binary_unary_op(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto next = set.peek_next();

    // Prefix Parsing
    if (const auto op_type = get_unary_op_type(next.get_type()); op_type.has_value())
    {
        set.next(); // Consume operator

        // Recursive call to handle chained unaries like !!x or - -x etc.
        // We call parse_inline_expression_part, which will call parse_binary_unary_op again.
        auto distinct_expr = parse_inline_expression_part(context, set);
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
            set.get_source(),
            next.get_source_position(),
            context,
            op_type.value(),
            std::move(distinct_expr),
            false // Prefix
        );
    }

    // Check for Postfix Unary (only Identifier supported for provided context/examples)
    // We can't easily hijack arbitrary expression parsing for postfix here without restructuring `parse_inline_expression_part`.
    // However, we can peek if we have Identifier -> PostfixOp
    if (set.peek_next_eq(TokenType::IDENTIFIER) && (
        // Check if the token AFTER identifier is ++ or --
        set.peek(1).get_type() == TokenType::DOUBLE_PLUS || set.peek(1).get_type() == TokenType::DOUBLE_MINUS))
    {
        const auto iden_tok = set.next();
        const auto iden_name = iden_tok.get_lexeme();

        const auto operation_tok = set.next();

        auto variable_def = context->lookup_variable(iden_name);
        if (!variable_def)
        {
            return std::nullopt;
        }
        const auto internal_name = variable_def->get_internal_symbol_name();

        UnaryOpType type = (operation_tok.get_type() == TokenType::DOUBLE_PLUS)
                               ? UnaryOpType::INCREMENT
                               : UnaryOpType::DECREMENT;

        return std::make_unique<AstUnaryOp>(
            set.get_source(),
            iden_tok.get_source_position(),
            context,
            type,
            std::make_unique<AstIdentifier>(
                set.get_source(),
                next.get_source_position(),
                context,
                iden_name,
                internal_name
            ),
            true // Postfix
        );
    }

    return std::nullopt;
}

bool AstUnaryOp::is_reducible()
{
    return this->get_operand().is_reducible();
}

IAstNode* AstUnaryOp::reduce()
{
    return this->get_operand().reduce();
}

std::string AstUnaryOp::to_string()
{
    return std::format(
        "UnaryOp({}{})",
        this->is_lsh() ? unary_op_type_to_str(this->get_op_type()) : this->get_operand().to_string(),
        this->is_lsh() ? this->get_operand().to_string() : unary_op_type_to_str(this->get_op_type())
    );
}
