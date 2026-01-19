#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/raw_ostream.h>

#include "ast/scope.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/primitive_type.h"

using namespace stride::ast;

llvm::Value* AstIdentifier::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    llvm::Value* val = nullptr;

    const auto internal_name = this->get_internal_name();

    if (const auto block = builder->GetInsertBlock())
    {
        if (const auto function = block->getParent())
        {
            val = function->getValueSymbolTable()->lookup(internal_name);
        }
    }

    // Check if it's a function argument
    if (auto* arg = llvm::dyn_cast_or_null<llvm::Argument>(val))
    {
        return arg;
    }

    if (auto* alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(val))
    {
        // Load the value from the allocated variable
        return builder->CreateLoad(alloca->getAllocatedType(), alloca, internal_name);
    }

    if (const auto global = module->getNamedGlobal(internal_name))
    {
        return builder->CreateLoad(global->getValueType(), global, internal_name);
    }

    if (auto* function = module->getFunction(internal_name))
    {
        return function;
    }

    llvm::errs() << "Unknown variable or function name: " << internal_name << "\n";
    return nullptr;
}

std::string AstIdentifier::to_string()
{
    return std::format("{} ({})", this->get_name().value, this->get_internal_name());
}

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


        return std::make_unique<AstVariableDeclaration>(
            this->source,
            this->source_offset,
            this->get_variable_name(),
            std::move(cloned_type),
            u_ptr<IAstNode>(std::move(reduced_value)),
            this->get_flags(),
            this->get_internal_name()
        ).release();
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

std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression_part(const Scope& scope, TokenSet& set)
{
    if (auto unary = parse_binary_unary_op(scope, set); unary.has_value())
    {
        return std::move(unary.value());
    }

    if (auto lit = parse_literal_optional(scope, set); lit.has_value())
    {
        return std::move(lit.value());
    }

    if (set.peak_next_eq(TokenType::LPAREN))
    {
        set.next();
        auto expr = parse_expression_ext(0, scope, set);
        set.expect(TokenType::RPAREN);
        return expr;
    }

    if (set.peak_next_eq(TokenType::IDENTIFIER))
    {
        if (set.peak(1).type == TokenType::LPAREN)
        {
            const auto reference_token = set.next();
            const auto name = reference_token.lexeme;
            auto function_parameter_set = collect_parenthesized_block(set);

            std::vector<std::unique_ptr<IAstNode>> function_parameter_nodes = {};

            // Parsing function parameter values
            if (function_parameter_set.has_value())
            {
                auto subset = function_parameter_set.value();
                auto initial_arg = parse_expression_ext(-1, scope, subset);
                function_parameter_nodes.push_back(std::move(initial_arg));

                while (subset.has_next())
                {
                    subset.expect(TokenType::COMMA);
                    auto next_arg = parse_standalone_expression_part(scope, subset);
                    function_parameter_nodes.push_back(std::move(next_arg));
                }
            }

            return std::make_unique<AstFunctionInvocation>(
                set.source(),
                reference_token.offset,
                Symbol(name),
                std::move(function_parameter_nodes)
            );
        }

        const auto reference_token = set.next();
        Symbol symbol(reference_token.lexeme);
        std::string internal_name;

        if (const auto sym_def = scope.get_symbol_globally(symbol); sym_def.has_value())
        {
            internal_name = sym_def->get_internal_name();
        }

        return std::make_unique<AstIdentifier>(
            set.source(),
            reference_token.offset,
            std::move(symbol),
            internal_name
        );
    }

    return nullptr;
}

/**
 * Parsing of an expression that does not require precedence, but has a LHS and RHS, and an operator.
 * This can be either binary expressions, e.g., 1 + 1, or comparative expressions, e.g., 1 < 2
 */
std::optional<std::unique_ptr<AstExpression>> parse_logical_or_comparative_op(
    const Scope& scope,
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
    const Scope& scope,
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
    auto logical_result = parse_logical_or_comparative_op(scope, set, std::move(lhs));

    if (logical_result.has_value())
    {
        return std::move(logical_result.value());
    }

    set.throw_error("Unexpected token in expression");
}

/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<AstExpression> stride::ast::parse_standalone_expression(const Scope& scope, TokenSet& tokens)
{
    return parse_expression_ext(
        SRFLAG_EXPR_ALLOW_VARIABLE_DECLARATION,
        scope,
        tokens
    );
}
