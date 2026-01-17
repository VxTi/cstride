#include "ast/nodes/literal_values.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_boolean_literal_optional(
    const Scope& scope,
    TokenSet& tokens
)
{
    if (tokens.peak_next_eq(TokenType::BOOLEAN_LITERAL))
    {
        const auto next = tokens.next();
        const bool value = next.lexeme == "true";

        return std::make_unique<AstBooleanLiteral>(value);
    }
    return std::nullopt;
}

std::string AstBooleanLiteral::to_string()
{
    return std::format("BooleanLiteral({})", value());
}

llvm::Value* AstBooleanLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    return llvm::ConstantInt::get(
        context,
        llvm::APInt(
            this->bit_count(),
            this->value(),
            true
        )
    );
}
