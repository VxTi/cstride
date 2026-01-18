#include "ast/nodes/literal_values.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_boolean_literal_optional(
    [[maybe_unused]]
    const Scope& scope,
    TokenSet& set
)
{
    if (const auto reference_token = set.peak_next(); reference_token.type == TokenType::BOOLEAN_LITERAL)
    {
        const auto next = set.next();
        const bool value = next.lexeme == "true";

        return std::make_unique<AstBooleanLiteral>(
            set.source(),
            reference_token.offset,
            value
        );
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
