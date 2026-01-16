#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Support/raw_ostream.h>

#include "ast/scope.h"
#include "ast/nodes/binary_op.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literals.h"
#include "ast/nodes/logical_op.h"
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
    return nullptr;
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

    for (const auto& child : children)
    {
        children_str += child->to_string() + ", ";
    }

    return std::format("Expression({})", children_str.substr(0, children_str.length() - 2));
}

bool stride::ast::is_logical_operator(const TokenType type)
{
    return type == TokenType::DOUBLE_AMPERSAND
        || type == TokenType::DOUBLE_PIPE
        || type == TokenType::LEQUALS
        || type == TokenType::GEQUALS
        || type == TokenType::NOT_EQUALS
        || type == TokenType::BANG_EQUALS;
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

std::unique_ptr<AstExpression> parse_primary(const Scope& scope, TokenSet& set)
{
    if (auto lit = AstLiteral::try_parse(scope, set))
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
                    auto next_arg = parse_primary(scope, subset);
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

std::unique_ptr<AstExpression> parse_binary_op(
    const Scope& scope,
    TokenSet& tokens,
    std::unique_ptr<AstExpression> lhs,
    int min_prec
)
{
    while (true)
    {
        const auto op = tokens.peak_next_type();
        const int precedence = operator_precedence(op);

        if (precedence < min_prec)
        {
            return lhs;
        }

        tokens.next();

        auto rhs = parse_primary(scope, tokens);
        if (!rhs)
        {
            return nullptr;
        }

        const auto next_op = tokens.peak_next().type;

        if (const int next_precedence = operator_precedence(next_op);
            precedence < next_precedence)
        {
            rhs = parse_binary_op(scope, tokens, std::move(rhs), precedence + 1);
        }

        if (is_logical_operator(op))
        {
            lhs = std::make_unique<AstLogicalOp>(std::move(lhs), op, std::move(rhs));
        }
        else
        {
            lhs = std::make_unique<AstBinaryOp>(std::move(lhs), op, std::move(rhs));
        }
    }
}

std::unique_ptr<AstExpression> stride::ast::parse_expression_ext(
    const int expression_type_flags,
    const Scope& scope,
    TokenSet& set
)
{
    if (is_variable_declaration(set))
    {
        if ((expression_type_flags & EXPRESSION_ALLOW_VARIABLE_DECLARATION) == 0)
        {
            set.throw_error("Variable declarations are not allowed in this context");
        }

        set.expect(TokenType::KEYWORD_LET);
        const auto variable_name_tok = set.expect(TokenType::IDENTIFIER);
        Symbol variable_name(variable_name_tok.lexeme);
        set.expect(TokenType::COLON);
        auto type = types::try_parse_type(set);

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

    auto lhs = parse_primary(scope, set);
    if (!lhs)
    {
        set.throw_error("Unexpected token");
    }

    return parse_binary_op(
        scope,
        set,
        std::move(lhs),
        1
    );
}

std::unique_ptr<AstExpression> stride::ast::parse_expression(const Scope& scope, TokenSet& tokens)
{
    return parse_expression_ext(
        EXPRESSION_ALLOW_VARIABLE_DECLARATION,
        scope,
        tokens
    );
}
