#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

#include "ast/optionals.h"
#include "ast/nodes/expression.h"

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
    const auto identifier_def = this->get_context()->lookup_variable(this->get_variable_name(), true);
    if (!identifier_def)
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Unable to reassign variable, variable '{}' not found", this->get_variable_name()),
            this->get_source_position()
        );
    }

    if (this->get_value() == nullptr) return;

    if (!identifier_def->get_type()->is_mutable())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is immutable and cannot be reassigned", this->get_variable_name()),
            this->get_source_position()
        );
    }

    const auto expression_type = infer_expression_type(this->get_context(), this->get_value());

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
                this->get_source_position(),
                this->get_context(),
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
    const std::shared_ptr<ParsingContext>& context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    auto& ctx = module->getContext();

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

    // Generate the RHS value
    llvm::Value* assign_val = this->get_value()->codegen(context, module, builder);

    if (!assign_val) return nullptr;

    const auto assign_ty = assign_val->getType();

    // Check if we're assigning to an optional type
    if (const auto variable_def = context->lookup_variable(this->get_variable_name());
        variable_def && variable_def->get_type()->is_optional())
    {
        if (llvm::Type* optional_ty = internal_type_to_llvm_type(variable_def->get_type(), module);
            assign_ty == optional_ty)
        {
            builder->CreateStore(assign_val, variable);
        }
        else
        {
            llvm::Value* has_value = nullptr;
            llvm::Value* value = nullptr;

            const auto value_ty = optional_ty->getStructElementType(OPT_IDX_ELEMENT_TYPE);

            if (llvm::isa<llvm::ConstantPointerNull>(assign_val))
            {
                has_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx), OPT_NO_VALUE);
                value = llvm::UndefValue::get(value_ty);
            }
            else
            {
                has_value = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx), OPT_HAS_VALUE);
                value = assign_val;

                if (value->getType() != value_ty &&
                    value->getType()->isIntegerTy() &&
                    value_ty->isIntegerTy())
                {
                    value = builder->CreateIntCast(value, value_ty, true);
                }
            }

            // Store has_value
            llvm::Value* has_value_ptr = builder->CreateStructGEP(optional_ty, variable, OPT_IDX_HAS_VALUE);
            builder->CreateStore(has_value, has_value_ptr);

            // Store value
            if (!llvm::isa<llvm::ConstantPointerNull>(assign_val))
            {
                llvm::Value* value_ptr = builder->CreateStructGEP(optional_ty, variable, OPT_IDX_ELEMENT_TYPE);
                builder->CreateStore(value, value_ptr);
            }
        }

        return variable;
    } // end optional type check

    const bool is_float = assign_ty->isFloatingPointTy();

    llvm::Value* finalValue = assign_val;

    // Handle Compound Assignments (+=, -=, etc.)
    if (this->get_operator() != MutativeAssignmentType::ASSIGN)
    {
        // Load the current value using the RHS type (works for 32/64 bit specifically)
        llvm::Value* cur_val = builder->CreateLoad(assign_ty, variable, "load_tmp");

        switch (this->get_operator())
        {
        case MutativeAssignmentType::ADD:
            finalValue = is_float
                             ? builder->CreateFAdd(cur_val, assign_val, "fadd_tmp")
                             : builder->CreateAdd(cur_val, assign_val, "add_tmp");
            break;
        case MutativeAssignmentType::SUBTRACT:
            finalValue = is_float
                             ? builder->CreateFSub(cur_val, assign_val, "fsub_tmp")
                             : builder->CreateSub(cur_val, assign_val, "sub_tmp");
            break;
        case MutativeAssignmentType::MULTIPLY:
            finalValue = is_float
                             ? builder->CreateFMul(cur_val, assign_val, "fmul_tmp")
                             : builder->CreateMul(cur_val, assign_val, "mul_tmp");
            break;
        case MutativeAssignmentType::DIVIDE:
            finalValue = is_float
                             ? builder->CreateFDiv(cur_val, assign_val, "fdiv_tmp")
                             : builder->CreateSDiv(cur_val, assign_val, "div_tmp");
            break;
        case MutativeAssignmentType::MODULO:
            finalValue = is_float
                             ? builder->CreateFRem(cur_val, assign_val, "frem_tmp")
                             : builder->CreateSRem(cur_val, assign_val, "mod_tmp");
            break;

        // Bitwise operations are generally not defined for floats in IR
        // and usually require a bitcast or are disallowed by the frontend.
        case MutativeAssignmentType::BITWISE_AND:
            finalValue = builder->CreateAnd(cur_val, assign_val, "and_tmp");
            break;
        case MutativeAssignmentType::BITWISE_OR:
            finalValue = builder->CreateOr(cur_val, assign_val, "or_tmp");
            break;
        case MutativeAssignmentType::BITWISE_XOR:
            finalValue = builder->CreateXor(cur_val, assign_val, "xor_tmp");
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
    return std::format(
        "VariableReassignment({}({}), {})",
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
    const std::shared_ptr<ParsingContext>& context,
    const std::string &variable_name,
    TokenSet& set
)
{
    // Can be either a singular field, e.g., a regular variable,
    // or member access, e.g., <member>.<field>
    const auto reference_token = set.peek_next();
    if (!is_variable_mutative_token(reference_token.get_type()))
    {
        return std::nullopt;
    }

    auto mutative_op = parse_mutative_assignment_type(set, reference_token);
    set.next();

    std::string reassignment_iden_name = variable_name;
    const auto reassign_internal_variable_name = context->lookup_variable(reassignment_iden_name, true);

    if (!reassign_internal_variable_name)
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Unable to reassign variable '{}', variable not found", reassignment_iden_name),
            reference_token.get_source_position()
        );
    }


    std::string reassign_internal_name = reassign_internal_variable_name->get_internal_symbol_name();

    auto expression = parse_inline_expression(context, set);

    return std::make_unique<AstVariableReassignment>(
        reference_token.get_source_position(),
        context,
        reassignment_iden_name,
        reassign_internal_name,
        mutative_op,
        std::move(expression)
    );
}
