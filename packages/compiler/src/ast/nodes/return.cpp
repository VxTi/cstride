#include "ast/nodes/return.h"
#include <format>
#include <llvm/IR/IRBuilder.h>
#include "ast/nodes/expression.h"

using namespace stride::ast;

bool stride::ast::is_return_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_RETURN);
}

std::unique_ptr<AstReturn> stride::ast::parse_return_statement(const std::shared_ptr<SymbolRegistry>& scope,
                                                               TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_RETURN);

    // If parsing a void return: `return`
    if (!set.has_next())
    {
        return std::make_unique<AstReturn>(
            set.get_source(),
            reference_token.get_source_position(),
            scope,
            nullptr
        );
    }

    auto value = parse_standalone_expression(scope, set);

    return std::make_unique<AstReturn>(
        set.get_source(),
        reference_token.get_source_position(),
        scope,
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
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    if (this->value())
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->value()))
        {
            llvm::Value* val = synthesisable->codegen(scope, module, context, builder);

            if (!val) return nullptr;

            // Create the return instruction
            return builder->CreateRet(val);
        }
    }
    return builder->CreateRetVoid();
}
