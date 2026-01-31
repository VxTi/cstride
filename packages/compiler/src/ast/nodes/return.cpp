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
    if (this->value())
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->value()))
        {
            llvm::Value* val = synthesisable->codegen(registry, module, builder);

            if (!val) return nullptr;

            // Implicitly unwrap optional if the return type is not optional
            // We need to know the function's return type.
            // For now, let's check if val is an optional struct and if it's being returned
            // to a function that doesn't expect it.
            // Since we don't easily have the current function's return type here,
            // we can check the LLVM function's return type.
            if (llvm::BasicBlock* cur_bb = builder->GetInsertBlock())
            {
                if (const llvm::Function* cur_func = cur_bb->getParent())
                {
                    if (const llvm::Type* ret_type = cur_func->getReturnType();
                        val->getType()->isStructTy() &&
                        !ret_type->isStructTy())
                    {
                        // Check if val is { i1, T }
                        if (val->getType()->getStructNumElements() == OPTIONAL_ELEMENT_COUNT &&
                            val->getType()->getStructElementType(OPTIONAL_HAS_VALUE_STRUCT_INDEX)->isIntegerTy(1))
                        {
                            val = builder->CreateExtractValue(
                                val,
                                {OPTIONAL_ELEMENT_TYPE_STRUCT_INDEX},
                                "unwrap_optional_ret"
                            );
                        }
                    }
                }
            }

            // Create the return instruction
            return builder->CreateRet(val);
        }
    }
    return builder->CreateRetVoid();
}
