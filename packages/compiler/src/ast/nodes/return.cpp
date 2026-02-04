#include "ast/nodes/return.h"
#include <format>
#include <llvm/IR/IRBuilder.h>

#include "ast/optionals.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

std::unique_ptr<AstReturn> stride::ast::parse_return_statement(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    // We can just do a quick check here, as we don't know yet whether in what context it's used.
    if (registry->get_current_scope_type() == ScopeType::GLOBAL ||
        registry->get_current_scope_type() == ScopeType::MODULE)
    {
        set.throw_error("Return statements are not allowed outside of functions");
    }
    const auto reference_token = set.next();

    // If parsing a void return: `return`
    if (!set.has_next())
    {
        set.expect(TokenType::SEMICOLON, "Expected ';' after return statement");
        return std::make_unique<AstReturn>(
            set.get_source(),
            reference_token.get_source_position(),
            registry,
            nullptr
        );
    }

    auto value = parse_inline_expression(registry, set);

    if (!value)
    {
        set.throw_error("Expected expression after return keyword");
    }

    set.expect(TokenType::SEMICOLON, "Expected ';' after return statement");

    const auto ref_pos = reference_token.get_source_position();
    const auto expr_pos = value->get_source_position();

    return std::make_unique<AstReturn>(
        set.get_source(),
        SourcePosition(ref_pos.offset, expr_pos.offset + expr_pos.length - ref_pos.offset),
        registry,
        std::move(value)
    );
}


void AstReturn::validate()
{
    auto registry = this->get_registry().get();

    while (registry->get_current_scope_type() != ScopeType::FUNCTION && registry->get_parent_registry() != nullptr)
    {
        registry = registry->get_parent_registry();
    }
    if (registry->get_current_scope_type() != ScopeType::FUNCTION)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            "Return statement cannot appear outside of functions",
            *this->get_source(),
            this->get_source_position()
        );
    }

    if (this->get_return_expr()) this->get_return_expr()->validate();
}

std::string AstReturn::to_string()
{
    return std::format(
        "Return(value: {})",
        _value ? _value->to_string() : "nullptr"
    );
}

llvm::Value* AstReturn::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    if (!this->get_return_expr())
    {
        return builder->CreateRetVoid();
    }

    llvm::Value* expr_return_val = this->get_return_expr()->codegen(registry, module, builder);

    if (!expr_return_val) return nullptr;

    llvm::BasicBlock* cur_bb = builder->GetInsertBlock();

    if (!cur_bb)
    {
        throw parsing_error(
            ErrorType::RUNTIME_ERROR,
            "Cannot return from a function that has no basic block",
            *this->get_source(),
            this->get_source_position()
        );
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
                expr_return_val = unwrap_optional_value(expr_return_val, builder);
            }
            // Function returns optional, but we have a non-optional (or nil) -> Wrap
            else if (!is_expr_optional && is_fn_return_optional)
            {
                expr_return_val = wrap_optional_value(expr_return_val, expected_return_ty, builder);
            }
        }
    }

    // Create the return instruction
    return builder->CreateRet(expr_return_val);
}
