#include <iostream>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::string format_int_conversion_error(const std::string& error, const TokenType type)
{
    if (error == "stoll: out of range")
    {
        return std::format("Number exceeds 64-bit integer limit. Max value is {}",
            type == TokenType::HEX_LITERAL ? "0x7FFFFFFFFFFFFFFF" : "9223372036854775807"
            );
    }

    if (error == "stoi: out of range")
    {
        return std::format("Number exceeds 32-bit integer limit. Max value is {}",
            type == TokenType::HEX_LITERAL ? "0x7FFFFFFF" : "2147483647"
        );
    }

    return error;// "Unknown error";
}

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_integer_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
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

            try
            {
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
                    context,
                    value,
                    bit_count
                );
            }
            catch (const std::exception& e)
            {
                const auto reason = format_int_conversion_error(e.what(), type);
                throw parsing_error(
                    ErrorType::SEMANTIC_ERROR,
                    reason,
                    *set.get_source(),
                    reference_token.get_source_position()
                );
            }
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
    const std::shared_ptr<ParsingContext>& context,
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
