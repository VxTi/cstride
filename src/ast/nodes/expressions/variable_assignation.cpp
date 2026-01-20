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

IAstNode* AstVariableReassignment::reduce()
{
    if (auto* reducible = dynamic_cast<IReducible*>(this->get_value()); reducible != nullptr)
    {
        return new AstVariableReassignment(
            this->source,
            this->source_offset,
            this->get_variable_name(),
            this->get_internal_name(),
            this->get_operator(),
            u_ptr<IAstNode>(reducible->reduce())
        );
    }

    return this;
}

llvm::Value* AstVariableReassignment::codegen(
    llvm::Module* module, llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    // Get the variable allocation
    llvm::Value* variable = module->getNamedGlobal(this->get_internal_name());
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

std::string AstVariableReassignment::to_string()
{
    return std::format("VariableAssignment({}({}), {})",
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
    switch (token.type)
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

option<u_ptr<AstVariableReassignment>> stride::ast::parse_variable_reassignment(const Scope& scope, TokenSet& set)
{
    // Can be either a singular field, e.g., a regular variable,
    // or member access, e.g., <member>.<field>
    if (is_property_accessor_statement(set))
    {
        const auto reference_token = set.peak_next();

        std::string reassignment_iden_name = reference_token.lexeme;
        auto reassignment_internal_name_def = scope.get_symbol(reference_token.lexeme);

        std::string reassign_internal_name = reassignment_internal_name_def.has_value()
                                                 ? reassignment_internal_name_def->get_internal_name()
                                                 : reassignment_iden_name;

        // Instead of moving the cursor over in the set, we peak forward.
        // This way, if it appears we don't have a mutative operation,
        // the standalone expression parser can continue with another expression variant
        int iterations = 0, offset = 1;
        while (true)
        {
            if (set.peak_eq(TokenType::DOT, offset) && set.peak_eq(TokenType::IDENTIFIER, offset + 1))
            {
                offset += 2;
                const auto accessor_token = set.peak(offset + 1);

                const auto accessor_internal_name_def = scope.get_symbol(accessor_token.lexeme);
                const std::string accessor_internal_name = accessor_internal_name_def.has_value()
                                                               ? accessor_internal_name_def->get_internal_name()
                                                               : accessor_token.lexeme;

                reassignment_iden_name += SR_PROPERTY_ACCESSOR_SEPARATOR + accessor_token.lexeme;
                reassign_internal_name += SR_PROPERTY_ACCESSOR_SEPARATOR + accessor_internal_name;
            }
            else break;

            if (++iterations > SR_EXPRESSION_MAX_IDENTIFIER_RESOLUTION)
            {
                set.throw_error("Maximum identifier resolution exceeded in variable reassignment");
            }
        }

        const auto mutative_token = set.peak(offset);

        if (!is_variable_mutative_token(mutative_token.type))
        {
            return std::nullopt;
        }

        set.skip(offset);

        auto mutative_op = parse_mutative_assignment_type(set, mutative_token);
        set.next();

        auto expression = parse_standalone_expression(scope, set);

        return std::make_unique<AstVariableReassignment>(
            set.source(),
            reference_token.offset,
            reassignment_iden_name,
            reassign_internal_name,
            mutative_op,
            std::move(expression)
        );
    }
    return std::nullopt;
}
