#include "ast/nodes/literals.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_char_literal_optional(
    const Scope& scope,
    TokenSet& tokens
)
{
    if (tokens.peak_next_eq(TokenType::CHAR_LITERAL))
    {
        const auto next = tokens.next();
        const char value = next.lexeme[0];

        return std::make_unique<AstCharLiteral>(value);
    }
    return std::nullopt;
}

std::string AstCharLiteral::to_string()
{
    return std::format("CharLiteral({})", value());
}

llvm::Value* AstCharLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    return llvm::ConstantInt::get(
        context,
        llvm::APInt(
            this->bit_count() * BITS_PER_BYTE,
            this->value(),
            true
        )
    );
}
