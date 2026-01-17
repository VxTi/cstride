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

    if (const auto block = builder->GetInsertBlock())
    {
        if (const auto function = block->getParent())
        {
            val = function->getValueSymbolTable()->lookup(name.value);
        }
    }

    if (auto* alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(val))
    {
        // Load the value from the allocated variable
        return builder->CreateLoad(alloca->getAllocatedType(), alloca, name.value.c_str());
    }

    if (auto* function = module->getFunction(name.value))
    {
        return function;
    }

    llvm::errs() << "Unknown variable or function name: " << name.value << "\n";
    return nullptr;
}

std::string AstIdentifier::to_string()
{
    return name.value;
}

llvm::Value* AstExpression::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irbuilder)
{
    throw parsing_error("Expression codegen not implemented, this must be implemented by subclasses");
}

bool AstVariableDeclaration::is_reducible()
{
    // Variables are reducible only if their initial value is reducible,
    // In the future we can also check whether variables are ever refereced,
    // in which case we can optimize away the variable declaration.
    if (const auto value = dynamic_cast<IReducible*>(this->initial_value.get()))
    {
        return value->is_reducible();
    }

    return false;
}

IAstNode* AstVariableDeclaration::reduce()
{
    if (this->is_reducible())
    {
        const auto reduced_value = dynamic_cast<IReducible*>(this->initial_value.get())->reduce();

        return std::make_unique<AstVariableDeclaration>(
            this->get_variable_name(),
            std::make_unique<types::AstType>(this->get_variable_type()),
            std::unique_ptr<IAstNode>(std::move(reduced_value))
        ).get();
    }
    return this;
}

bool stride::ast::can_parse_expression(const TokenSet& tokens)
{
    const auto type = tokens.peak_next().type;

    if (is_literal(type)) return true;

    switch (type)
    {
    case TokenType::IDENTIFIER:      // <something>
    case TokenType::LPAREN:          // (
    case TokenType::RPAREN:          // )
    case TokenType::BANG:            // !
    case TokenType::MINUS:           // -
    case TokenType::PLUS:            // +
    case TokenType::TILDE:           // ~
    case TokenType::CARET:           // ^
    case TokenType::LSQUARE_BRACKET: // [
    case TokenType::RSQUARE_BRACKET: // ]
    case TokenType::STAR:            // *
    case TokenType::AMPERSAND:       // &
    case TokenType::DOUBLE_MINUS:    // --
    case TokenType::DOUBLE_PLUS:     // ++
    case TokenType::KEYWORD_NIL:     // nil
        return true;
    default: break;
    }
    return false;
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

std::optional<LogicalOpType> stride::ast::get_logical_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::DOUBLE_AMPERSAND: return LogicalOpType::AND;
    case TokenType::DOUBLE_PIPE: return LogicalOpType::OR;
    default: return std::nullopt;
    }
}

std::optional<ComparisonOpType> stride::ast::get_comparative_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::DOUBLE_EQUALS: return ComparisonOpType::EQUAL;
    case TokenType::BANG_EQUALS: return ComparisonOpType::NOT_EQUAL;
    case TokenType::LARROW: return ComparisonOpType::LESS_THAN;
    case TokenType::LEQUALS: return ComparisonOpType::LESS_THAN_OR_EQUAL;
    case TokenType::RARROW: return ComparisonOpType::GREATER_THAN;
    case TokenType::GEQUALS: return ComparisonOpType::GREATER_THAN_OR_EQUAL;
    default: return std::nullopt;
    }
}

std::optional<BinaryOpType> stride::ast::get_binary_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::STAR: return BinaryOpType::MULTIPLY;
    case TokenType::PLUS: return BinaryOpType::ADD;
    case TokenType::MINUS: return BinaryOpType::SUBTRACT;
    case TokenType::SLASH: return BinaryOpType::DIVIDE;
    default: return std::nullopt;
    }
}

int stride::ast::operator_precedence(const TokenType type)
{
    switch (type)
    {
    case TokenType::STAR:
    case TokenType::SLASH:
        return 2;
    case TokenType::PLUS:
    case TokenType::MINUS:
        return 1;
    default:
        return 0;
    }
}

std::unique_ptr<AstExpression> parse_standalone_expression_part(const Scope& scope, TokenSet& set)
{
    if (auto lit = parse_literal_optional(scope, set))
    {
        return std::move(*lit);
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
            auto name = set.next().lexeme;
            auto function_parameter_set = collect_token_subset(set, TokenType::LPAREN, TokenType::RPAREN);

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

            return std::make_unique<AstFunctionInvocation>(Symbol(name), std::move(function_parameter_nodes));
        }
        auto tok = set.next();
        return std::make_unique<AstIdentifier>(Symbol(tok.lexeme));
    }

    return nullptr;
}

/**
 * Parsing of an expression that does not require precedence, but has a LHS and RHS, and an operator.
 * This can be either binary expressions, e.g., 1 + 1, or comparative expressions, e.g., 1 < 2
 */
std::optional<std::unique_ptr<AstExpression>> parse_logical_or_comparative_op(const Scope& scope, TokenSet& set)
{
    auto lhs = parse_standalone_expression_part(scope, set);
    if (!lhs)
    {
        return std::nullopt;
    }

    const auto op_token = set.peak_next_type();

    if (auto logical_op = get_logical_op_type(op_token); logical_op.has_value())
    {
        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstLogicalOp>(std::move(lhs), logical_op.value(), std::move(rhs));
    }

    if (auto comparative_op = get_comparative_op_type(op_token); comparative_op.has_value())
    {
        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        return std::make_unique<AstComparisonOp>(std::move(lhs), comparative_op.value(), std::move(rhs));
    }

    return std::nullopt;
}

/**
 * This parses expressions that requires precedence, such as binary expressions.
 * These are binary expressions, e.g., 1 + 1, 1 - 1, 1 * 1, 1 / 1, 1 % 1
 */
std::optional<std::unique_ptr<AstExpression>> parse_binary_op(
    const Scope& scope,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs,
    const int min_precedence
)
{
    for (;;)
    {
        const auto op = set.peak_next_type();
        const int precedence = operator_precedence(op);

        if (precedence < min_precedence)
        {
            return lhs;
        }

        set.next();

        auto rhs = parse_standalone_expression_part(scope, set);
        if (!rhs)
        {
            return std::nullopt;
        }

        const auto next_op = set.peak_next().type;

        if (const int next_precedence = operator_precedence(next_op);
            precedence < next_precedence)
        {
            if (auto rhs_opt = parse_binary_op(scope, set, std::move(rhs), precedence + 1); rhs_opt.has_value())
            {
                rhs = std::move(rhs_opt.value());
            }
            else
            {
                return std::nullopt;
            }
        }

        if (auto binary_op = get_binary_op_type(op); binary_op.has_value())
        {
            lhs = std::make_unique<AstBinaryOp>(std::move(lhs), binary_op.value(), std::move(rhs));
        }
        else
        {
            set.throw_error("Unexpected token in expression");
        }
    }
}

std::unique_ptr<AstVariableDeclaration> try_parse_variable_declaration(
    const int expression_type_flags,
    const Scope& scope,
    TokenSet& set
)
{
    if ((expression_type_flags & EXPRESSION_ALLOW_VARIABLE_DECLARATION) == 0)
    {
        set.throw_error("Variable declarations are not allowed in this context");
    }

    set.expect(TokenType::KEYWORD_LET);
    const auto variable_name_tok = set.expect(TokenType::IDENTIFIER);
    Symbol variable_name(variable_name_tok.lexeme);
    set.expect(TokenType::COLON);
    auto type = types::parse_primitive_type(set);

    set.expect(TokenType::EQUALS);

    auto value = parse_expression_ext(
        EXPRESSION_VARIABLE_ASSIGNATION,
        scope, set
    );

    // If it's not an inline variable declaration (e.g., in a for loop),
    // we expect a semicolon at the end.
    if ((expression_type_flags & EXPRESSION_INLINE_VARIABLE_DECLARATION) == 0)
    {
        set.expect(TokenType::SEMICOLON);
    }

    scope.try_define_scoped_symbol(*set.source(), variable_name_tok, variable_name);

    return std::make_unique<AstVariableDeclaration>(
        variable_name,
        std::move(type),
        std::move(value)
    );
}

std::unique_ptr<AstExpression> stride::ast::parse_expression_ext(
    const int expression_type_flags,
    const Scope& scope,
    TokenSet& set
)
{
    if (is_variable_declaration(set))
    {
        return try_parse_variable_declaration(
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

    if (auto binary_op = parse_binary_op(
        scope,
        set,
        std::move(lhs),
        1
    ); binary_op.has_value())
    {
        return std::move(binary_op.value());
    }

    if (auto logical_op = parse_logical_or_comparative_op(scope, set);
        logical_op.has_value() && logical_op != nullptr)
    {
        return std::move(logical_op).value();
    }

    set.throw_error("Unexpected token in expression");
}

/**
 * General expression parsing. These can occur in global / function scopes
 */
std::unique_ptr<AstExpression> stride::ast::parse_expression(const Scope& scope, TokenSet& tokens)
{
    return parse_expression_ext(
        EXPRESSION_ALLOW_VARIABLE_DECLARATION,
        scope,
        tokens
    );
}
