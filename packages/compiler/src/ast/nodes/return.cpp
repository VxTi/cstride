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
    const auto reference_token = set.expect(TokenType::KEYWORD_RETURN);

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

    return std::make_unique<AstReturn>(
        set.get_source(),
        reference_token.get_source_position(),
        registry,
        std::move(value)
    );
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
    if (!this->value())
    {
        return builder->CreateRetVoid();
    }

    llvm::Value* val = this->value()->codegen(registry, module, builder);

    if (!val) return nullptr;

    llvm::BasicBlock* cur_bb = builder->GetInsertBlock();

    if (!cur_bb) return nullptr;

    // Implicitly unwrap optional if the return type is not optional
    // or wrap if the return type is optional.
    if (const llvm::Function* cur_func = cur_bb->getParent())
    {
        if (llvm::Type* ret_type = cur_func->getReturnType();
            val->getType() != ret_type)
        {
            // Case 1: Function returns non-optional, but we have an optional -> Unwrap
            if (val->getType()->isStructTy() &&
                !ret_type->isStructTy() &&
                val->getType()->getStructNumElements() == OPTIONAL_ELEMENT_COUNT &&
                val->getType()->getStructElementType(OPTIONAL_HAS_VALUE_STRUCT_INDEX)->isIntegerTy(1))
            {
                val = builder->CreateExtractValue(
                    val,
                    {OPTIONAL_ELEMENT_TYPE_STRUCT_INDEX},
                    "unwrap_optional_ret"
                );
            }
            // Case 2: Function returns optional, but we have a non-optional (or nil) -> Wrap
            else if (ret_type->isStructTy() &&
                !val->getType()->isStructTy() &&
                ret_type->getStructNumElements() == OPTIONAL_ELEMENT_COUNT &&
                ret_type->getStructElementType(OPTIONAL_HAS_VALUE_STRUCT_INDEX)->isIntegerTy(1))
            {
                llvm::Type* inner_type = ret_type->getStructElementType(
                    OPTIONAL_ELEMENT_TYPE_STRUCT_INDEX
                );

                if (llvm::isa<llvm::ConstantPointerNull>(val))
                {
                    // nil -> { i1 false, undef T }
                    llvm::Value* ret_val = llvm::UndefValue::get(ret_type);
                    ret_val = builder->CreateInsertValue(
                        ret_val,
                        builder->getInt1(OPTIONAL_NO_VALUE),
                        {OPTIONAL_HAS_VALUE_STRUCT_INDEX}
                    );
                    val = ret_val;
                }
                else
                {
                    // value -> { i1 true, value }
                    if (val->getType()->isIntegerTy() && inner_type->isIntegerTy() && val->getType() !=
                        inner_type)
                    {
                        val = builder->CreateIntCast(val, inner_type, true);
                    }

                    if (val->getType() == inner_type)
                    {
                        llvm::Value* ret_val = llvm::UndefValue::get(ret_type);
                        ret_val = builder->CreateInsertValue(
                            ret_val,
                            builder->getInt1(OPTIONAL_HAS_VALUE),
                            {OPTIONAL_HAS_VALUE_STRUCT_INDEX}
                        );
                        ret_val = builder->CreateInsertValue(
                            ret_val,
                            val,
                            {OPTIONAL_ELEMENT_TYPE_STRUCT_INDEX}
                        );
                        val = ret_val;
                    }
                }
            }

            // Final check after potential wrapping/unwrapping
            if (val->getType() != ret_type && val->getType()->isIntegerTy() && ret_type->isIntegerTy())
            {
                val = builder->CreateIntCast(val, ret_type, true);
            }
        }
    }

    // Create the return instruction
    return builder->CreateRet(val);
}
