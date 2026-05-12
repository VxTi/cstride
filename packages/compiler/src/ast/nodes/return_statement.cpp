#include "ast/nodes/return_statement.h"

#include "errors.h"
#include "ast/optionals.h"
#include "ast/symbol_table.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<AstReturnStatement> stride::ast::parse_return_statement(TokenSet& set)
{
    const auto reference_token = set.next();
    const auto& ref_pos = reference_token.get_source_position();

    std::optional<std::unique_ptr<IAstExpression>> return_value = std::nullopt;

    // If we don't see a semicolon immediately after, we expect a return expression.
    if (!set.peek_next_eq(TokenType::SEMICOLON))
    {
        return_value = parse_inline_expression(set);
        if (!return_value)
        {
            set.throw_error("Expected expression after return keyword");
        }
    }

    const auto& end_pos =
        set
       .expect(TokenType::SEMICOLON, "Expected ';' after return statement")
       .get_source_position();

    return std::make_unique<AstReturnStatement>(
        SourcePosition::join(ref_pos, end_pos),
        std::move(return_value)
    );
}

void AstReturnStatement::validate(SymbolTable* symbol_table)
{
    while (symbol_table->get_context_type() != ContextType::FUNCTION &&
        symbol_table->get_parent_symbol_table() != nullptr)
    {
        symbol_table = symbol_table->get_parent_symbol_table().get();
    }
    if (symbol_table->get_context_type() != ContextType::FUNCTION)
    {
        throw stride_error(
            ErrorType::SYNTAX_ERROR,
            "Return statement cannot appear outside of functions",
            this->get_source_position());
    }

    if (this->get_return_expression().has_value())
        this->get_return_expression().value()->validate(symbol_table);
}

std::string AstReturnStatement::to_string()
{
    return std::format(
        "Return(value: {})",
        this->_value.has_value()
        ? this->_value.value()->to_string()
        : "void"
    );
}

llvm::Value* AstReturnStatement::codegen(
    SymbolTable* symbol_table,
    llvm::Module* module, llvm::IRBuilderBase* builder
)
{
    // If no return expression is provided, default to void
    if (!this->get_return_expression().has_value())
    {
        return builder->CreateRetVoid();
    }

    llvm::Value* return_value = this->get_return_expression().value()->codegen(
        symbol_table,
        module, builder
    );

    llvm::BasicBlock* cur_bb = builder->GetInsertBlock();
    if (!return_value || !cur_bb)
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            "Cannot return from a function that has no basic block",
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
            return_value->getType() != expected_return_ty)
        {
            const auto is_expr_optional = is_optional_wrapped_type(return_value->getType());

            // Function returns non-optional, but we have an optional -> Unwrap
            if (const auto is_fn_return_optional = is_optional_wrapped_type(expected_return_ty);
                is_expr_optional && !is_fn_return_optional)
            {
                return_value = unwrap_optional_value(return_value, builder);
            }
            // Function returns optional, but we have a non-optional (or nil) -> Wrap
            else if (!is_expr_optional && is_fn_return_optional)
            {
                return_value = wrap_optional_value(return_value, expected_return_ty, builder);
            }
            else
            {
                // We have a different type, so we have to check whether we can
                // cast the expression's type to the function's return type
                if (expected_return_ty->isIntegerTy() && return_value->getType()->isIntegerTy() &&
                    expected_return_ty->getIntegerBitWidth() != return_value->getType()->getIntegerBitWidth())
                {
                    return_value = builder->CreateIntCast(return_value, expected_return_ty, true);
                }
                else if (expected_return_ty->isFloatTy() && return_value->getType()->isFloatTy() &&
                    expected_return_ty->getPrimitiveSizeInBits() != return_value->getType()->getPrimitiveSizeInBits())
                {
                    return_value = builder->CreateFPExt(return_value, expected_return_ty, "fpext");
                }
                else
                {
                    throw stride_error(
                        ErrorType::COMPILATION_ERROR,
                        "Cannot cast return value to function return type",
                        this->get_source_position()
                    );
                }
            }
        }
    }
    else
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            "Cannot determine the function return type for return statement",
            this->get_source_position()
        );
    }

    // Create the return instruction
    return builder->CreateRet(return_value);
}

std::unique_ptr<IAstNode> AstReturnStatement::clone()
{
    return std::make_unique<AstReturnStatement>(
        this->get_source_position(),
        this->_value.has_value()
        ? std::make_optional(this->_value.value()->clone_as<IAstExpression>())
        : std::nullopt
    );
}
