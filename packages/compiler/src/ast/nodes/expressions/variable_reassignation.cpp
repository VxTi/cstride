#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

bool AstVariableReassignment::is_reducible()
{
    if (auto* reducible = dynamic_cast<IReducible*>(this->get_value()); reducible != nullptr)
    {
        return reducible->is_reducible();
    }

    return false;
}

void AstVariableReassignment::validate()
{
    const auto identifier_def = this->get_registry()->field_lookup(this->get_variable_name());
    if (!identifier_def)
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Unable to reassign variable, variable '{}' not found", this->get_variable_name()),
            *this->get_source(),
            this->get_source_position()
        );
    }

    if (this->get_value() == nullptr) return;

    if (!identifier_def->get_type()->is_mutable())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is immutable and cannot be reassigned", this->get_variable_name()),
            *this->get_source(),
            this->get_source_position()
        );
    }

    const auto expression_type = infer_expression_type(this->get_registry(), this->get_value());

    /*if (identifier_def->get_type() != expression_type.get())
    {
        throw parsing_error(
            make_ast_error(
                *this->get_source,
                this->source_offset,
                std::format(
                    "Type mismatch when reassigning variable '{}', expected type '{}', got type '{}'",
                    this->get_variable_name(),
                    identifier_def->get_type()->to_string(),
                    expression_type.get()->to_string()
                )
            )
        );
    }*/
}

IAstNode* AstVariableReassignment::reduce()
{
    if (auto* reducible = dynamic_cast<IReducible*>(this->get_value()); reducible != nullptr)
    {
        const auto reduced = reducible->reduce();

        if (auto* reduced_expr = dynamic_cast<AstExpression*>(reduced); reduced_expr != nullptr)
        {
            return std::make_unique<AstVariableReassignment>(
                this->get_source(),
                this->get_source_position(),
                this->get_registry(),
                this->get_variable_name(),
                this->get_internal_name(),
                this->get_operator(),
                std::unique_ptr<AstExpression>(reduced_expr)
            ).release();
        }
    }

    return this;
}

llvm::Value* AstVariableReassignment::codegen(
    const std::shared_ptr<SymbolRegistry>& registry, llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    // Locate the variable (AllocaInst or GlobalVariable)
    llvm::Value* variable = builder->GetInsertBlock()->getValueSymbolTable()->lookup(this->get_internal_name());
    if (!variable)
    {
        variable = module->getNamedGlobal(this->get_variable_name());
    }
    if (!variable)
    {
        variable = module->getNamedGlobal(this->get_internal_name());
    }

    if (!variable)
    {
        throw std::runtime_error(std::format("Variable '{}' not found", this->get_variable_name()));
    }

    auto* synthesisable = dynamic_cast<ISynthesisable*>(this->get_value());
    if (!synthesisable) return nullptr;

    // Generate the RHS value
    llvm::Value* rhsValue = synthesisable->codegen(registry, module, builder);

    // Check if we're assigning to an optional type
    if (const auto identifier_def = registry->field_lookup(this->get_variable_name());
        identifier_def && identifier_def->get_type()->is_optional())
    {
        llvm::Value* has_value = nullptr;
        llvm::Value* value = nullptr;
        llvm::Type* struct_type = internal_type_to_llvm_type(identifier_def->get_type(), module);

        if (llvm::isa<llvm::ConstantPointerNull>(rhsValue))
        {
            has_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(module->getContext()), 0);
            value = llvm::UndefValue::get(struct_type->getStructElementType(1));
        }
        else
        {
            has_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(module->getContext()), 1);
            value = rhsValue;

            if (llvm::Type* expected_val_type = struct_type->getStructElementType(1);
                value->getType() != expected_val_type &&
                value->getType()->isIntegerTy() &&
                expected_val_type->isIntegerTy()
            )
            {
                value = builder->CreateIntCast(value, expected_val_type, true);
            }
        }

        // Store has_value
        llvm::Value* has_value_ptr = builder->CreateStructGEP(struct_type, variable, 0);
        builder->CreateStore(has_value, has_value_ptr);

        // Store value
        if (!llvm::isa<llvm::ConstantPointerNull>(rhsValue))
        {
            llvm::Value* value_ptr = builder->CreateStructGEP(struct_type, variable, 1);
            builder->CreateStore(value, value_ptr);
        }

        return variable;
    }

    const bool is_float = rhsValue->getType()->isFloatingPointTy();

    llvm::Value* finalValue = rhsValue;

    // Handle Compound Assignments (+=, -=, etc.)
    if (this->get_operator() != MutativeAssignmentType::ASSIGN)
    {
        // Load the current value using the RHS type (works for 32/64 bit specifically)
        llvm::Value* currentValue = builder->CreateLoad(rhsValue->getType(), variable, "load_tmp");

        switch (this->get_operator())
        {
        case MutativeAssignmentType::ADD:
            finalValue = is_float
                             ? builder->CreateFAdd(currentValue, rhsValue, "fadd_tmp")
                             : builder->CreateAdd(currentValue, rhsValue, "add_tmp");
            break;
        case MutativeAssignmentType::SUBTRACT:
            finalValue = is_float
                             ? builder->CreateFSub(currentValue, rhsValue, "fsub_tmp")
                             : builder->CreateSub(currentValue, rhsValue, "sub_tmp");
            break;
        case MutativeAssignmentType::MULTIPLY:
            finalValue = is_float
                             ? builder->CreateFMul(currentValue, rhsValue, "fmul_tmp")
                             : builder->CreateMul(currentValue, rhsValue, "mul_tmp");
            break;
        case MutativeAssignmentType::DIVIDE:
            finalValue = is_float
                             ? builder->CreateFDiv(currentValue, rhsValue, "fdiv_tmp")
                             : builder->CreateSDiv(currentValue, rhsValue, "div_tmp");
            break;
        case MutativeAssignmentType::MODULO:
            finalValue = is_float
                             ? builder->CreateFRem(currentValue, rhsValue, "frem_tmp")
                             : builder->CreateSRem(currentValue, rhsValue, "mod_tmp");
            break;

        // Bitwise operations are generally not defined for floats in IR
        // and usually require a bitcast or are disallowed by the frontend.
        case MutativeAssignmentType::BITWISE_AND:
            finalValue = builder->CreateAnd(currentValue, rhsValue, "and_tmp");
            break;
        case MutativeAssignmentType::BITWISE_OR:
            finalValue = builder->CreateOr(currentValue, rhsValue, "or_tmp");
            break;
        case MutativeAssignmentType::BITWISE_XOR:
            finalValue = builder->CreateXor(currentValue, rhsValue, "xor_tmp");
            break;
        default:
            break;
        }
    }

    builder->CreateStore(finalValue, variable);

    return finalValue;
}

std::string AstVariableReassignment::to_string()
{
    return std::format("VariableReassignment({}({}), {})",
                       this->get_variable_name(),
                       this->get_internal_name(),
                       this->get_value()->to_string()
    );
}

/**
 * @brief Checks if a given token type represents a mutative operation on a variable.
 *
 * This function determines whether the provided token type corresponds to an
 * assignment or compound assignment operation that modifies a variable.
 *
 * @param type The token type to check.
 * @return Returns true if the token type modifies a variable, otherwise false.
 */
bool is_variable_mutative_token(const TokenType type)
{
    switch (type)
    {
    case TokenType::EQUALS:
    case TokenType::PLUS_EQUALS:
    case TokenType::MINUS_EQUALS:
    case TokenType::STAR_EQUALS:
    case TokenType::SLASH_EQUALS:
    case TokenType::PERCENT_EQUALS:
    case TokenType::AMPERSAND_EQUALS:
    case TokenType::PIPE_EQUALS:
    case TokenType::CARET_EQUALS:
        return true;
    default: return false;
    }
}

MutativeAssignmentType parse_mutative_assignment_type(const TokenSet& set, const Token& token)
{
    switch (token.get_type())
    {
    case TokenType::EQUALS:
        return MutativeAssignmentType::ASSIGN;
    case TokenType::PLUS_EQUALS:
        return MutativeAssignmentType::ADD;
    case TokenType::MINUS_EQUALS:
        return MutativeAssignmentType::SUBTRACT;
    case TokenType::STAR_EQUALS:
        return MutativeAssignmentType::MULTIPLY;
    case TokenType::SLASH_EQUALS:
        return MutativeAssignmentType::DIVIDE;
    case TokenType::PERCENT_EQUALS:
        return MutativeAssignmentType::MODULO;
    case TokenType::AMPERSAND_EQUALS:
        return MutativeAssignmentType::BITWISE_AND;
    case TokenType::PIPE_EQUALS:
        return MutativeAssignmentType::BITWISE_OR;
    case TokenType::CARET_EQUALS:
        return MutativeAssignmentType::BITWISE_XOR;
    default:
        set.throw_error(
            token,
            stride::ErrorType::SYNTAX_ERROR,
            "Expected mutative assignment operator"
        );
    }
}

std::optional<std::unique_ptr<AstVariableReassignment>> stride::ast::parse_variable_reassignment(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    // Can be either a singular field, e.g., a regular variable,
    // or member access, e.g., <member>.<field>
    if (!is_variable_reassignment_statement(set)) return std::nullopt;

    const auto reference_token = set.peek_next();

    std::string reassignment_iden_name = reference_token.get_lexeme();
    const auto reassign_internal_variable_name = registry->field_lookup(reassignment_iden_name);

    if (!reassign_internal_variable_name)
    {
        return std::nullopt;
    }


    std::string reassign_internal_name = reassign_internal_variable_name->get_internal_symbol_name();

    // Instead of moving the cursor over in the set, we peek forward.
    // This way, if it appears we don't have a mutative operation,
    // the standalone expression parser can continue with another expression variant
    const auto mutative_token = set.peek(1);

    if (!is_variable_mutative_token(mutative_token.get_type()))
    {
        return std::nullopt;
    }

    set.skip(1);

    auto mutative_op = parse_mutative_assignment_type(set, mutative_token);
    set.next();

    auto expression = parse_standalone_expression(registry, set);

    return std::make_unique<AstVariableReassignment>(
        set.get_source(),
        reference_token.get_source_position(),
        registry,
        reassignment_iden_name,
        reassign_internal_name,
        mutative_op,
        std::move(expression)
    );
}
