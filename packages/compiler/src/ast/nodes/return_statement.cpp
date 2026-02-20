#include "ast/nodes/return_statement.h"

#include "ast/optionals.h"

#include <format>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<AstReturnStatement> stride::ast::parse_return_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    // We can just do a quick check here, as we don't know yet whether in what context it's used.
    if (context->get_current_scope_type() == ScopeType::GLOBAL ||
        context->get_current_scope_type() == ScopeType::MODULE)
    {
        set.throw_error(
            "Return statements are not allowed outside of functions");
    }
    const auto reference_token = set.next();
    const auto& ref_pos = reference_token.get_source_fragment();

    std::unique_ptr<AstExpression> return_value = nullptr;

    // If we don't see a semicolon immediately after, we expect a return expression.
    if (!set.peek_next_eq(TokenType::SEMICOLON))
    {
        return_value = parse_inline_expression(context, set);
        if (!return_value)
        {
            set.throw_error("Expected expression after return keyword");
        }
    }

    const auto& end_tok = set.expect(TokenType::SEMICOLON, "Expected ';' after return statement");
    const auto& end_pos = end_tok.get_source_fragment();

    const auto position = SourceFragment(
        set.get_source(),
        ref_pos.offset,
        ref_pos.offset + end_pos.offset - end_pos.length
    );

    return std::make_unique<AstReturnStatement>(
        position,
        context,
        std::move(return_value)
    );
}


void AstReturnStatement::validate()
{
    auto context = this->get_context().get();

    while (context->get_current_scope_type() != ScopeType::FUNCTION &&
        context->get_parent_registry() != nullptr)
    {
        context = context->get_parent_registry();
    }
    if (context->get_current_scope_type() != ScopeType::FUNCTION)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            "Return statement cannot appear outside of functions",
            this->get_source_fragment());
    }

    if (this->get_return_expr())
        this->get_return_expr()->validate();
}

std::string AstReturnStatement::to_string()
{
    return std::format("Return(value: {})",
                       _value ? _value->to_string() : "nullptr");
}

llvm::Value* AstReturnStatement::codegen(
    llvm::Module* module,
    llvm::IRBuilder<>* builder)
{
    if (!this->get_return_expr())
    {
        return builder->CreateRetVoid();
    }

    llvm::Value* expr_return_val = this->get_return_expr()->codegen(
        module,
        builder
    );

    if (!expr_return_val)
        return nullptr;

    llvm::BasicBlock* cur_bb = builder->GetInsertBlock();

    if (!cur_bb)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Cannot return from a function that has no basic block",
            this->get_source_fragment());
    }

    // Implicitly unwrap optional if the return type is not optional
    // or wrap if the return type is optional.
    if (const llvm::Function* cur_func = cur_bb->getParent())
    {
        // If the received return type doesn't match the function return type, we may need to
        // wrap it into an optional container
        if (llvm::Type* expected_return_ty = cur_func->getReturnType();
            expr_return_val->getType() != expected_return_ty)
        {
            const auto is_expr_optional = is_optional_wrapped_type(expr_return_val->getType());

            // Function returns non-optional, but we have an optional -> Unwrap
            if (const auto is_fn_return_optional = is_optional_wrapped_type(expected_return_ty);
                is_expr_optional && !is_fn_return_optional)
            {
                expr_return_val = unwrap_optional_value(
                    expr_return_val,
                    builder
                );
            }
            // Function returns optional, but we have a non-optional (or nil) -> Wrap
            else if (!is_expr_optional && is_fn_return_optional)
            {
                expr_return_val = wrap_optional_value(
                    expr_return_val,
                    expected_return_ty,
                    builder
                );
            }
        }
    }
    else
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Cannot determine the function return type for return statement",
            this->get_source_fragment()
        );
    }

    // Create the return instruction
    return builder->CreateRet(expr_return_val);
}
