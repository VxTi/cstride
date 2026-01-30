#include <iostream>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_integer_literal_optional(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();

    switch (const auto type = reference_token.get_type())
    {
    case TokenType::INTEGER_LITERAL:
    case TokenType::LONG_INTEGER_LITERAL:
    case TokenType::HEX_LITERAL:
        {
            const std::string input = reference_token.get_lexeme();
            set.skip(1);


            const long long int value =
                type == TokenType::HEX_LITERAL
                    ? std::stoi(input, nullptr, 16)
                    : type == TokenType::LONG_INTEGER_LITERAL
                    ? std::stoll(input)
                    : std::stoi(input);
            const short bit_count =
                reference_token.get_type() == TokenType::LONG_INTEGER_LITERAL
                    ? 64
                    : INFER_INT_BIT_COUNT(value);

            return std::make_unique<AstIntLiteral>(
                set.get_source(),
                reference_token.get_source_position(),
                scope,
                value,
                bit_count
            );
        }
    default:
        return std::nullopt;
    }
}

std::string AstIntLiteral::to_string()
{
    return std::format("IntLiteral({})", value());
}

llvm::Value* AstIntLiteral::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    return llvm::ConstantInt::get(
        module->getContext(),
        llvm::APInt(
            this->bit_count(),
            this->value(),
            this->is_signed()
        )
    );
}
