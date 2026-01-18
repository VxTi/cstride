#include "ast/nodes/literal_values.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_integer_literal_optional(
    const Scope& scope,
    TokenSet& set
)
{
    const auto reference_token = set.peak_next();
    int offset = 0;

    if (reference_token.type == TokenType::PLUS || reference_token.type == TokenType::MINUS)
    {
        offset++;
    }

    if (set.peak(offset).type == TokenType::INTEGER_LITERAL)
    {
        set.skip(offset + 1);

        return std::make_unique<AstIntegerLiteral>(
            set.source(),
            reference_token.offset,
            std::move(std::stoi(reference_token.lexeme)),
            SRFLAG_INT_UNSIGNED
        );
    }
    return std::nullopt;
}

std::string AstIntegerLiteral::to_string()
{
    return std::format("IntLiteral({})", value());
}

llvm::Value* AstIntegerLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    return llvm::ConstantInt::get(
        context,
        llvm::APInt(
            this->bit_count(),
            this->value(),
            this->is_signed()
        )
    );
}
