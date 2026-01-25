#include <iostream>

#include <llvm/IR/Constants.h>
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_integer_literal_optional(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.peak_next();
    int offset = 0;

    if (reference_token.type == TokenType::PLUS || reference_token.type == TokenType::MINUS)
    {
        offset++;
    }


    if (const auto is_hex_type = set.peak(offset).type == TokenType::HEX_LITERAL;
        set.peak(offset).type == TokenType::INTEGER_LITERAL || is_hex_type)
    {
        const std::string input = offset > 0 ? reference_token.lexeme + set.peak(offset).lexeme :  reference_token.lexeme;
        set.skip(offset + 1);


        const int value = is_hex_type
                              ? std::stoi(input, nullptr, 16)
                              : std::stoi(input);

        return std::make_unique<AstIntLiteral>(
            set.source(),
            reference_token.offset,
            scope,
            value,
            SRFLAG_INT_UNSIGNED
        );
    }
    return std::nullopt;
}

std::string AstIntLiteral::to_string()
{
    return std::format("IntLiteral({})", value());
}

llvm::Value* AstIntLiteral::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
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
