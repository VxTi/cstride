#include "errors.h"
#include "ast/casting.h"
#include "ast/optionals.h"
#include "ast/parsing_context.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

using namespace stride::ast;

void AstVariableReassignment::resolve_forward_references(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    this->get_value()->resolve_forward_references(module, builder);
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
    case TokenType::DOUBLE_GT_EQ:
    case TokenType::DOUBLE_LT_EQ:
        return true;
    default:
        return false;
    }
}

bool is_bitwise_mutative_operation(const MutativeAssignmentType type)
{
    switch (type)
    {
    case MutativeAssignmentType::BITWISE_AND:
    case MutativeAssignmentType::BITWISE_OR:
    case MutativeAssignmentType::BITWISE_XOR:
    case MutativeAssignmentType::BITWISE_RIGHT_SHIFT:
    case MutativeAssignmentType::BITWISE_LEFT_SHIFT:
        return true;
    default:
        return false;
    }
}

MutativeAssignmentType parse_mutative_assignment_type(const Token& token)
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
    case TokenType::DOUBLE_GT_EQ:
        return MutativeAssignmentType::BITWISE_RIGHT_SHIFT;
    case TokenType::DOUBLE_LT_EQ:
        return MutativeAssignmentType::BITWISE_LEFT_SHIFT;
    default:
        TokenSet::throw_error(
            token,
            stride::ErrorType::SYNTAX_ERROR,
            "Expected mutative assignment operator");
    }
}

std::optional<std::unique_ptr<AstVariableReassignment>> stride::ast::parse_variable_reassignment(
    const std::shared_ptr<ParsingContext>& context,
    const std::string& variable_name,
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

    auto operation = parse_mutative_assignment_type(reference_token);
    set.next();

    auto expression = parse_inline_expression(context, set);

    if (!expression)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            "Expected expression after variable reassignment",
            reference_token.get_source_fragment()
        );
    }

    return std::make_unique<AstVariableReassignment>(
        reference_token.get_source_fragment(),
        context,
        variable_name,
        operation,
        std::move(expression)
    );
}

void AstVariableReassignment::validate()
{
    this->_value->validate();
    const auto identifier_def = this->get_context()->lookup_variable(this->get_variable_name(), true);

    if (!identifier_def)
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format(
                "Unable to reassign variable, variable '{}' not found",
                this->get_variable_name()),
            this->get_source_fragment());
    }

    this->_internal_name = identifier_def->get_internal_symbol_name();

    if (is_bitwise_mutative_operation(this->_operator) &&
        this->_value->get_type()->is_primitive() &&
        cast_type<AstPrimitiveType*>(this->_value->get_type())->is_fp())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Bitwise mutative operations are not supported on floating point types, got type '{}'",
                this->_value->get_type()->to_string()),
            this->get_source_fragment());
    }

    if (!identifier_def->get_type()->is_mutable())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format(
                "Variable '{}' is immutable and cannot be reassigned",
                this->get_variable_name()),
            this->get_source_fragment());
    }

    if (!identifier_def->get_type()->equals(*this->get_value()->get_type()))
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Type mismatch when reassigning variable '{}', expected type '{}', got type '{}'",
                this->get_variable_name(),
                identifier_def->get_type()->to_string(),
                this->get_value()->get_type()->to_string()
            ),
            this->get_source_fragment()
        );
    }
}

llvm::Value* AstVariableReassignment::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    const auto internal_name = this->_internal_name.value();

    // Locate the variable (AllocaInst or GlobalVariable)
    llvm::Value* variable = builder->GetInsertBlock()->getValueSymbolTable()->lookup(internal_name);
    if (!variable)
    {
        variable = module->getNamedGlobal(this->get_variable_name());
    }

    if (!variable)
    {
        variable = module->getNamedGlobal(internal_name);
    }

    if (!variable)
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Variable '{}' not found", this->get_variable_name()),
            this->get_source_fragment()
        );
    }

    // Save the insertion point before codegen, as callable types (lambdas) may change it
    llvm::BasicBlock* saved_block = builder->GetInsertBlock();
    const auto saved_point = builder->GetInsertPoint();

    // Generate the RHS value
    llvm::Value* assign_val = this->get_value()->codegen(module, builder);

    // Restore the insertion point after codegen
    if (saved_block && saved_block != builder->GetInsertBlock())
    {
        builder->SetInsertPoint(saved_block, saved_point);
    }

    if (!assign_val)
    {
        return nullptr;
    }

    const auto assign_ty = assign_val->getType();

    // Check if we're assigning to an optional type
    if (const auto variable_def = this->get_context()->lookup_variable(this->get_variable_name());
        variable_def != nullptr &&
        variable_def->get_type()->is_optional())
    {
        llvm::Type* optional_ty = type_to_llvm_type(variable_def->get_type(), module);
        llvm::Value* wrapped_val = wrap_optional_value(assign_val, optional_ty, builder);
        builder->CreateStore(wrapped_val, variable);

        return variable;
    }

    const bool is_float = assign_ty->isFloatingPointTy();

    llvm::Value* finalValue = assign_val;

    // Handle Compound Assignments (+=, -=, etc.)
    if (this->get_operator() != MutativeAssignmentType::ASSIGN)
    {
        // Load the current value using the RHS type (works for 32/64 bit specifically)
        llvm::Value* cur_val = builder->CreateLoad(
            assign_ty,
            variable,
            "load_tmp");

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
        case MutativeAssignmentType::BITWISE_RIGHT_SHIFT:
            finalValue = builder->CreateAShr(cur_val, assign_val, "ashr_tmp");
            break;
        case MutativeAssignmentType::BITWISE_LEFT_SHIFT:
            finalValue = builder->CreateLShr(cur_val, assign_val, "lshr_tmp");
            break;
        default:
            break;
        }
    }

    builder->CreateStore(finalValue, variable);

    return finalValue;
}

std::unique_ptr<IAstNode> AstVariableReassignment::clone()
{
    return std::make_unique<AstVariableReassignment>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_variable_name(),
        this->get_operator(),
        this->get_value()->clone_as<IAstExpression>()
    );
}

bool AstVariableReassignment::is_reducible()
{
    return this->get_value()->is_reducible();
}

std::optional<std::unique_ptr<IAstNode>> AstVariableReassignment::reduce()
{
    if (!this->get_value()->is_reducible())
        return std::nullopt;

    const auto reduced = this->get_value()->reduce();

    if (auto* reduced_expr = dynamic_cast<IAstExpression*>(reduced.value().get()))
    {
        return std::make_unique<AstVariableReassignment>(
            this->get_source_fragment(),
            this->get_context(),
            this->get_variable_name(),
            this->get_operator(),
            std::unique_ptr<IAstExpression>(reduced_expr)
        );
    }

    return std::nullopt;
}

std::string AstVariableReassignment::to_string()
{
    return std::format(
        "VariableReassignment({}({}), {})",
        this->get_variable_name(),
        this->_internal_name.value_or(""),
        this->get_value()->to_string());
}
