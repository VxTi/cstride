#include "ast/nodes/return.h"
#include <format>
#include <llvm/IR/IRBuilder.h>

#include "ast/optionals.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

bool stride::ast::is_return_statement(const TokenSet& tokens)
{
    return tokens.peek_next_eq(TokenType::KEYWORD_RETURN);
}

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
        return std::make_unique<AstReturn>(
            set.get_source(),
            reference_token.get_source_position(),
            registry,
            nullptr
        );
    }

    auto value = parse_standalone_expression(registry, set);

    if (!value)
    {
        set.throw_error("Expected expression after return keyword");
    }

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

    llvm::Value* expr_ret_val = this->get_return_expr()->codegen(registry, module, builder);

    if (!expr_ret_val) return nullptr;

    llvm::BasicBlock* cur_bb = builder->GetInsertBlock();

    if (!cur_bb) return nullptr;

    // Implicitly unwrap optional if the return type is not optional
    // or wrap if the return type is optional.
    if (const llvm::Function* cur_func = cur_bb->getParent())
    {
        if (llvm::Type* fn_return_type = cur_func->getReturnType();
            expr_ret_val->getType() != fn_return_type)
        {
            // Case 1: Function returns non-optional, but we have an optional -> Unwrap
            if (expr_ret_val->getType()->isStructTy() &&
                !fn_return_type->isStructTy() &&
                expr_ret_val->getType()->getStructNumElements() == OPT_ELEMENT_COUNT &&
                expr_ret_val->getType()->getStructElementType(OPT_IDX_HAS_VALUE)->isIntegerTy(OPT_HAS_VALUE_BIT_COUNT))
            {
                expr_ret_val = builder->CreateExtractValue(
                    expr_ret_val,
                    {OPT_IDX_ELEMENT_TYPE},
                    "unwrap_optional_ret"
                );
            }
            // Case 2: Function returns optional, but we have a non-optional (or nil) -> Wrap
            else if (fn_return_type->isStructTy() &&
                !expr_ret_val->getType()->isStructTy() &&
                fn_return_type->getStructNumElements() == OPT_ELEMENT_COUNT &&
                fn_return_type->getStructElementType(OPT_IDX_HAS_VALUE)->isIntegerTy(OPT_HAS_VALUE_BIT_COUNT))
            {
                llvm::Type* optional_ty = fn_return_type->getStructElementType(
                    OPT_IDX_ELEMENT_TYPE
                );

                if (llvm::isa<llvm::ConstantPointerNull>(expr_ret_val))
                {
                    // nil -> { i1 false, undef T }
                    llvm::Value* ret_optional_val = llvm::UndefValue::get(fn_return_type);
                    ret_optional_val = builder->CreateInsertValue(
                        ret_optional_val,
                        builder->getInt1(OPT_NO_VALUE),
                        {OPT_IDX_HAS_VALUE}
                    );
                    expr_ret_val = ret_optional_val;
                }
                else
                {
                    // value -> { i1 true, value }
                    if (expr_ret_val->getType()->isIntegerTy()
                        && optional_ty->isIntegerTy()
                        && expr_ret_val->getType() != optional_ty)
                    {
                        expr_ret_val = builder->CreateIntCast(expr_ret_val, optional_ty, true);
                    }

                    if (expr_ret_val->getType() == optional_ty)
                    {
                        llvm::Value* ret_val = llvm::UndefValue::get(fn_return_type);
                        ret_val = builder->CreateInsertValue(
                            ret_val,
                            builder->getInt1(OPT_HAS_VALUE),
                            {OPT_IDX_HAS_VALUE}
                        );
                        ret_val = builder->CreateInsertValue(
                            ret_val,
                            expr_ret_val,
                            {OPT_IDX_ELEMENT_TYPE}
                        );
                        expr_ret_val = ret_val;
                    }
                }
            }

            // Final check after potential wrapping/unwrapping
            if (expr_ret_val->getType() != fn_return_type && expr_ret_val->getType()->isIntegerTy() && fn_return_type->isIntegerTy())
            {
                expr_ret_val = builder->CreateIntCast(expr_ret_val, fn_return_type, true);
            }
        }
    }

    // Create the return instruction
    return builder->CreateRet(expr_ret_val);
}
