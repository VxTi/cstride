#include "ast/nodes/expression.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

#include "ast/nodes/binary_op.h"
#include "ast/nodes/literals.h"
#include "ast/nodes/primitive_type.h"

using namespace stride::ast;

llvm::Value* AstIdentifier::codegen(llvm::Module* module, llvm::LLVMContext& context)
{
    return nullptr;
}

std::string AstIdentifier::to_string()
{
    return name.value;
}

llvm::Value* AstExpression::codegen(llvm::Module* module, llvm::LLVMContext& context)
{
    return nullptr;
}

llvm::Value* AstVariableDeclaration::codegen(llvm::Module* module, llvm::LLVMContext& context)
{
    // Generate code for the initial value
    llvm::Value* init_value = nullptr;
    if (this->initial_value != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->initial_value.get()))
        {
            init_value = synthesisable->codegen(module, context);
        }
    }

    // Get the LLVM type for the variable
    llvm::Type* var_type = types::ast_type_to_llvm(this->variable_type.get(), context);

    // Create an alloca instruction for the variable
    llvm::IRBuilder<> builder(context);
    // Find the entry block of the current function
    llvm::Function* current_function = builder.GetInsertBlock()->getParent();
    llvm::BasicBlock& entry_block = current_function->getEntryBlock();
    builder.SetInsertPoint(&entry_block, entry_block.begin());

    llvm::AllocaInst* alloca = builder.CreateAlloca(var_type, nullptr, this->variable_name.value);

    // Store the initial value if it exists
    if (init_value != nullptr)
    {
        builder.CreateStore(init_value, alloca);
    }

    return alloca;
}

std::string AstVariableDeclaration::to_string()
{
    return std::format(
        "VariableDeclaration({}, {}, {})",
        variable_name.value,
        variable_type->to_string(),
        initial_value ? initial_value->to_string() : "nil"
    );
}

bool AstExpression::can_parse(const TokenSet& tokens)
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

bool is_variable_declaration(const TokenSet& set)
{
    return (set.peak_next_eq(TokenType::IDENTIFIER) || is_primitive(set.peak_next_type()))
        && set.peak(1).type == TokenType::IDENTIFIER
        && set.peak(2).type == TokenType::EQUALS;
}

int get_precedence(TokenType type)
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

std::unique_ptr<AstExpression> parse_primary(const Scope& scope, TokenSet& tokens)
{
    if (auto lit = AstLiteral::try_parse(scope, tokens))
    {
        return std::move(*lit);
    }

    if (tokens.peak_next_eq(TokenType::LPAREN))
    {
        tokens.next();
        auto expr = AstExpression::try_parse_expression(0, scope, tokens);
        tokens.expect(TokenType::RPAREN);
        return expr;
    }

    if (tokens.peak_next_eq(TokenType::IDENTIFIER))
    {
        if (tokens.peak(1).type == TokenType::LPAREN)
        {
            auto name = tokens.next().lexeme;
            tokens.expect(TokenType::LPAREN);
            tokens.expect(TokenType::RPAREN);
            return std::make_unique<AstFunctionInvocation>(Symbol(name));
        }
        auto tok = tokens.next();
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
        int prec = get_precedence(op);

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
        int next_prec = get_precedence(next_op);

        if (prec < next_prec)
        {
            rhs = parse_binary_op(scope, tokens, std::move(rhs), prec + 1);
        }

        lhs = std::make_unique<AstBinaryOp>(std::move(lhs), op, std::move(rhs));
    }
}

std::unique_ptr<AstExpression> AstExpression::try_parse_expression(
    const int expression_type_flags,
    const Scope& scope,
    TokenSet& set
)
{
    if (is_variable_declaration(set))
    {
        if ((expression_type_flags & EXPRESSION_VARIABLE_DECLARATION) != 0)
        {
            set.throw_error("Variable declarations are not allowed in this context");
        }

        auto type = types::try_parse_type(set);
        auto variable_name_tok = set.expect(TokenType::IDENTIFIER);
        Symbol variable_name(variable_name_tok.lexeme);

        set.expect(TokenType::EQUALS);

        auto value = try_parse_expression(
            EXPRESSION_VARIABLE_ASSIGNATION,
            scope, set
        );

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
        return nullptr;
    }

    return parse_binary_op(
        scope,
        set,
        std::move(lhs),
        1
    );
}

std::unique_ptr<AstExpression> AstExpression::try_parse(const Scope& scope, TokenSet& tokens)
{
    return try_parse_expression(
        EXPRESSION_VARIABLE_DECLARATION |
        EXPRESSION_INLINE_VARIABLE_DECLARATION,
        scope,
        tokens
    );
}
