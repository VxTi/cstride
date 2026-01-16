#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

#include "ast/nodes/binary_op.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literals.h"
#include "ast/nodes/primitive_type.h"

using namespace stride::ast;

llvm::Value* AstIdentifier::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
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
    case TokenType::IDENTIFIER: // <something>
    case TokenType::LPAREN: // (
    case TokenType::RPAREN: // )
    case TokenType::BANG: // !
    case TokenType::MINUS: // -
    case TokenType::PLUS: // +
    case TokenType::TILDE: // ~
    case TokenType::CARET: // ^
    case TokenType::LSQUARE_BRACKET: // [
    case TokenType::RSQUARE_BRACKET: // ]
    case TokenType::STAR: // *
    case TokenType::AMPERSAND: // &
    case TokenType::DOUBLE_MINUS: // --
    case TokenType::DOUBLE_PLUS: // ++
    case TokenType::KEYWORD_NIL: // nil
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
        auto expr = try_parse_expression_ext(0, scope, set);
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
                auto initial_arg = try_parse_expression_ext(-1, scope, subset);
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
        auto op = tokens.peak_next().type;
        int prec = operator_precedence(op);

        if (prec < min_prec)
        {
            return lhs;
        }

        tokens.next();

        auto rhs = parse_primary(scope, tokens);
        if (!rhs)
        {
            return nullptr;
        }

        auto next_op = tokens.peak_next().type;
        int next_prec = operator_precedence(next_op);

        if (prec < next_prec)
        {
            rhs = parse_binary_op(scope, tokens, std::move(rhs), prec + 1);
        }

        lhs = std::make_unique<AstBinaryOp>(std::move(lhs), op, std::move(rhs));
    }
}

std::unique_ptr<AstExpression> stride::ast::try_parse_expression_ext(
    const int expression_type_flags,
    const Scope& scope,
    TokenSet& set
)
{
    if (is_variable_declaration(set))
    {
        if ((expression_type_flags & EXPRESSION_VARIABLE_DECLARATION) == 0)
        {
            set.throw_error("Variable declarations are not allowed in this context");
        }

        set.expect(TokenType::KEYWORD_LET);
        const auto variable_name_tok = set.expect(TokenType::IDENTIFIER);
        Symbol variable_name(variable_name_tok.lexeme);
        set.expect(TokenType::COLON);
        auto type = types::try_parse_type(set);

        set.expect(TokenType::EQUALS);

        auto value = try_parse_expression_ext(
            EXPRESSION_VARIABLE_ASSIGNATION,
            scope, set
        );

        set.expect(TokenType::SEMICOLON);

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

std::unique_ptr<AstExpression> stride::ast::try_parse_expression(const Scope& scope, TokenSet& tokens)
{
    return try_parse_expression_ext(
        EXPRESSION_VARIABLE_DECLARATION |
        EXPRESSION_INLINE_VARIABLE_DECLARATION,
        scope,
        tokens
    );
}