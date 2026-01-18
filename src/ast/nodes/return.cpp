#include "ast/nodes/return.h"
#include "ast/nodes/expression.h"
#include <llvm/IR/IRBuilder.h>
#include <format>

using namespace stride::ast;

bool stride::ast::is_return_statement(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_RETURN);
}

std::unique_ptr<AstReturn> stride::ast::parse_return_statement(const Scope& scope, TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_RETURN);

    // If parsing a void return: `return;`
    if (set.peak_next_eq(TokenType::SEMICOLON))
    {
        set.expect(TokenType::SEMICOLON);
        return std::make_unique<AstReturn>(
            set.source(),
            reference_token.offset,
            nullptr
        );
    }

    auto value = parse_standalone_expression(scope, set);
    set.expect(TokenType::SEMICOLON);

    return std::make_unique<AstReturn>(
        set.source(),
        reference_token.offset,
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

llvm::Value* AstReturn::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    if (this->value())
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->value()))
        {
            llvm::Value* val = synthesisable->codegen(module, context, builder);
            // When using create ret, we also return the instruction as a value
            return builder->CreateRet(val);
        }
    }
    return builder->CreateRetVoid();
}
