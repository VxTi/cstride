#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/raw_ostream.h>

#include "ast/scope.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/functions.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

llvm::Value* AstExpression::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irBuilder)
{
    throw parsing_error("Expression codegen not implemented, this must be implemented by subclasses");
}

bool AstVariableDeclaration::is_reducible()
{
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

std::string AstExpression::to_string()
{
    std::string children_str;

    for (const auto& child : this->children())
    {
        children_str += child->to_string() + ", ";
    }

    return std::format("Expression({})", children_str.substr(0, children_str.length() - 2));
}

std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression_part(Scope& scope, TokenSet& set)
{
    if (auto lit = parse_literal_optional(scope, set);
        lit.has_value())
    {
        return std::move(lit.value());
    }

    if (auto unary = parse_binary_unary_op(scope, set);
        unary.has_value())
    {
        return std::move(unary.value());
    }


    if (auto reassignment = parse_variable_reassignment(scope, set);
        reassignment.has_value())
    {
        return std::move(reassignment.value());
    }

    if (set.peak_next_eq(TokenType::LPAREN))
    {
        set.next();
        auto expr = parse_standalone_expression_part(scope, set);
        set.expect(TokenType::RPAREN);
        return expr;
    }

    if (set.peak_next_eq(TokenType::IDENTIFIER))
    {
        if (set.peak(1).type == TokenType::LPAREN)
        {
            const auto reference_token = set.next();
            const auto candidate_function_name = reference_token.lexeme;
            auto function_parameter_set = collect_parenthesized_block(set);

            std::vector<std::unique_ptr<AstExpression>> function_arg_nodes = {};
            std::vector<std::shared_ptr<IAstInternalFieldType>> parameter_types = {};

            // Parsing function parameter values
            if (function_parameter_set.has_value())
            {
                auto subset = function_parameter_set.value();
                auto initial_arg = parse_standalone_expression_part(scope, subset);

                parameter_types.push_back(std::move(resolve_expression_internal_type(scope, &*initial_arg)));
                function_arg_nodes.push_back(std::move(initial_arg));

                while (subset.has_next())
                {
                    subset.expect(TokenType::COMMA);
                    auto next_arg = parse_standalone_expression_part(scope, subset);

                    parameter_types.push_back(std::move(resolve_expression_internal_type(scope, &*next_arg)));
                    function_arg_nodes.push_back(std::move(next_arg));
                }
            }

            std::string internal_fn_name = resolve_internal_function_name(
                parameter_types,
                candidate_function_name
            );

            return std::make_unique<AstFunctionInvocation>(
                set.source(),
                reference_token.offset,
                candidate_function_name,
                internal_fn_name,
                std::move(function_arg_nodes)
            );
        }

        const auto reference_token = set.next();
        std::string identifier_name = reference_token.lexeme;
        std::string internal_name;

        if (const auto variable_definition = scope.get_variable_def(identifier_name);
            variable_definition != nullptr)
        {
            internal_name = variable_definition->get_internal_symbol_name();
        }

        return std::make_unique<AstIdentifier>(
            set.source(),
            reference_token.offset,
            std::move(identifier_name),
            internal_name
        );
    }

    throw parsing_error(
        make_ast_error(
            *set.source(),
            set.peak_next().offset,
            "Invalid expression"
        )
    );
}

/**
 * Parsing of an expression that does not require precedence, but has a LHS and RHS, and an operator.
 * This can be either binary expressions, e.g., 1 + 1, or comparative expressions, e.g., 1 < 2
 */
std::optional<std::unique_ptr<AstExpression>> parse_logical_or_comparative_op(
    Scope& scope,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs
)
{
    const auto reference_token = set.peak_next();

    if (auto logical_op = get_logical_op_type(reference_token.type); logical_op.has_value())
    {
        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstLogicalOp>(
            set.source(),
            reference_token.offset,
            std::move(lhs),
            logical_op.value(),
            std::move(rhs)
        );
    }

    if (auto comparative_op = get_comparative_op_type(reference_token.type); comparative_op.has_value())
    {
        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstComparisonOp>(
            set.source(),
            reference_token.offset,
            std::move(lhs),
            comparative_op.value(),
            std::move(rhs)
        );
    }

    return lhs;
}


std::unique_ptr<AstExpression> stride::ast::parse_expression_ext(
    const int expression_type_flags,
    Scope& scope,
    TokenSet& set
)
{
    if (is_variable_declaration(set))
    {
        return parse_variable_declaration(
            expression_type_flags,
            scope,
            set
        );
    }

    auto lhs = parse_standalone_expression_part(scope, set);
    if (!lhs)
    {
        set.throw_error("Unexpected token in expression");
    }

    // Attempt to parse arithmetic binary operations first

    // If we have a result from arithmetic parsing, update lhs.
    // Note: parse_arithmetic_binary_op returns the lhs if no arithmetic op is found,
    // so it effectively passes through unless an error occurs (returns nullopt).
    if (auto arithmetic_result = parse_arithmetic_binary_op(scope, set, std::move(lhs), 1); arithmetic_result.
        has_value())
    {
        lhs = std::move(arithmetic_result.value());
    }
    else
    {
        // This case handles errors during arithmetic parsing (e.g. valid LHS but invalid RHS)
        set.throw_error("Invalid arithmetic expression");
    }

    // Now attempt to parse logical or comparative operations using the result

    if (auto logical_result = parse_logical_or_comparative_op(scope, set, std::move(lhs));
        logical_result.has_value())
    {
        return std::move(logical_result.value());
    }

    set.throw_error("Unexpected token in expression");
}

/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression(Scope& scope, TokenSet& set)
{
    return parse_expression_ext(
        SRFLAG_EXPR_ALLOW_VARIABLE_DECLARATION,
        scope,
        set
    );
}

std::string stride::ast::parse_property_accessor_statement(Scope& scope, TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::IDENTIFIER);
    auto identifier_name = reference_token.lexeme;

    int iterations = 0;
    while (true)
    {
        if (set.peak_next_eq(TokenType::DOT) && set.peak_eq(TokenType::IDENTIFIER, 1))
        {
            set.next();
            const auto accessor_token = set.expect(TokenType::IDENTIFIER);
            identifier_name += SR_PROPERTY_ACCESSOR_SEPARATOR + accessor_token.lexeme;
        }
        else break;

        if (++iterations > SR_EXPRESSION_MAX_IDENTIFIER_RESOLUTION)
        {
            set.throw_error("Maximum identifier resolution exceeded");
        }
    }

    return identifier_name;
}

bool stride::ast::is_property_accessor_statement(const TokenSet& set)
{
    const bool initial_identifier = set.peak_next_eq(TokenType::IDENTIFIER);
    const bool is_followup_accessor = set.peak_eq(TokenType::DOT, 1)
        && set.peak_eq(TokenType::IDENTIFIER, 2);

    return initial_identifier && (is_followup_accessor || true);
}


/**
 * Resolves the internal field type for a given literal expression.
 * 
 * This function examines a literal AST node and determines its corresponding internal field type
 * representation. It performs runtime type identification (via dynamic_cast) to determine the
 * specific kind of literal (string, floating-point, integer, character, or boolean) and creates
 * an appropriate AstPrimitiveFieldType instance with the correct primitive type and bit count.
 * 
 * The function handles the following literal types:
 * - AstStringLiteral: Resolves to STRING primitive type with bit count of 1
 * - AstFpLiteral: Resolves to FLOAT32 or FLOAT64 depending on bit count (>4 bytes = FLOAT64)
 * - AstIntLiteral: Resolves to INT32 or INT64 depending on bit count (>32 bits = INT64)
 * - AstCharLiteral: Resolves to CHAR primitive type with the literal's bit count
 * - AstBooleanLiteral: Resolves to BOOL primitive type with the literal's bit count
 * 
 * @param scope The current scope context (used for symbol resolution, though not directly used in this function)
 * @param literal Pointer to the AstLiteral node whose type needs to be resolved
 * @return A unique pointer to an IAstInternalFieldType representing the resolved type
 * @throws std::runtime_error If the literal type cannot be resolved (unknown literal subtype)
 */
std::unique_ptr<IAstInternalFieldType> resolve_expression_literal_internal_type(const Scope& scope, AstLiteral* literal)
{
    // "string" -> <string> (primitive)
    if (const auto* str = dynamic_cast<AstStringLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            str->source,
            str->source_offset,
            PrimitiveType::STRING,
            1
        );
    }

    // 1.325 -> <fp32 / fp64>
    if (const auto* fp_lit = dynamic_cast<AstFpLiteral*>(literal))
    {
        auto type = PrimitiveType::FLOAT32;
        if (fp_lit->bit_count() > 4)
        {
            type = PrimitiveType::FLOAT64;
        }

        return std::make_unique<AstPrimitiveFieldType>(
            fp_lit->source,
            fp_lit->source_offset,
            type,
            fp_lit->bit_count()
        );
    }

    // 1 -> <int32 / int64>
    if (const auto* int_lit = dynamic_cast<AstIntLiteral*>(literal))
    {
        auto type = int_lit->bit_count() > 32 ? PrimitiveType::INT64 : PrimitiveType::INT32;
        return std::make_unique<AstPrimitiveFieldType>(
            int_lit->source,
            int_lit->source_offset,
            type,
            int_lit->bit_count()
        );
    }

    // 'a' -> <char>
    if (const auto* char_lit = dynamic_cast<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            char_lit->source,
            char_lit->source_offset,
            PrimitiveType::CHAR,
            char_lit->bit_count()
        );
    }

    // true / false -> <bool>
    if (const auto* bool_lit = dynamic_cast<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            bool_lit->source,
            bool_lit->source_offset,
            PrimitiveType::BOOL,
            bool_lit->bit_count()
        );
    }

    throw std::runtime_error(
        stride::make_ast_error(
            *literal->source,
            literal->source_offset,
            "Unable to resolve expression literal type"
        )
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::resolve_expression_internal_type(
    Scope& scope,
    AstExpression* expr
)
{
    // If the provided expression is already an IAstInternalFieldType, we can easily resolve it
    if (auto* literal = dynamic_cast<AstLiteral*>(expr))
    {
        return resolve_expression_literal_internal_type(scope, literal);
    }

    if (const auto* operation = dynamic_cast<AstBinaryArithmeticOp*>(expr))
    {
        std::unique_ptr<IAstInternalFieldType> lhs_type = resolve_expression_internal_type(
            scope, &operation->get_left());
        std::unique_ptr<IAstInternalFieldType> rhs_type = resolve_expression_internal_type(
            scope, &operation->get_right());

        // If both types are equal, then we know we've got the final type.
        if (*lhs_type == *rhs_type)
        {
            return std::move(lhs_type);
        }

        // Otherwise, we'll have to check which type is dominant.
        // For example, if we receive an arithmetic operation with two integer types of different
        // bit sizes, we'll return the integer type with the larger size.
        // This ensures we reserve the right amount of memory.
        return get_dominant_type(lhs_type.get(), rhs_type.get());
    }

    // We continue the same process as before for other expression types.


    // For unary expressions it's quite straight-forward; we'll just resolve the operand type.
    if (const auto* operation = dynamic_cast<AstUnaryOp*>(expr))
    {
        return resolve_expression_internal_type(scope, &operation->get_operand());
    }

    if (dynamic_cast<AstLogicalOp*>(expr) || dynamic_cast<AstComparisonOp*>(expr))
    {
        // For logical operations (&&, ||) and comparative ops, the return type is always a boolean.
        return std::make_unique<AstPrimitiveFieldType>(
            expr->source,
            expr->source_offset,
            PrimitiveType::BOOL,
            1,
            0
        );
    }

    if (const auto* operation = dynamic_cast<AstVariableReassignment*>(expr))
    {
        return resolve_expression_internal_type(scope, operation->get_value());
    }

    if (const auto* operation = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        const auto declared_type = operation->get_variable_type();
        const auto value_type = resolve_expression_internal_type(scope, operation->get_initial_value().get());

        if (*declared_type == *value_type)
        {
            return declared_type->clone();
        }

        return get_dominant_type(declared_type, value_type.get());
    }

    throw parsing_error("Unable to resolve expression type");
}
