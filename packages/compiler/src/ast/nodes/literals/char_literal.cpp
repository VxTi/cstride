#include "ast/nodes/literal_values.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_char_literal_optional(
    std::shared_ptr<Scope> scope,
    TokenSet& set
)
{
    if (const auto reference_token = set.peak_next(); reference_token.type == TokenType::CHAR_LITERAL)
    {
        const auto next = set.next();
        const char value = next.lexeme[0];

        return std::make_unique<AstCharLiteral>(
            set.source(),
            reference_token.offset,
            scope,
            value
        );
    }
    return std::nullopt;
}

std::string AstCharLiteral::to_string()
{
    return std::format("CharLiteral({})", value());
}

llvm::Value* AstCharLiteral::codegen(const std::shared_ptr<Scope>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
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
