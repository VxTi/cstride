#include "errors.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <iostream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::string format_int_conversion_error(
    const std::string& error,
    const TokenType type)
{
    if (error == "stoll: out of range")
    {
        return std::format(
            "Number exceeds 64-bit integer limit. Max value is {}",
            type == TokenType::HEX_LITERAL
            ? "0x7FFFFFFFFFFFFFFF"
            : "9223372036854775807");
    }

    if (error == "stoi: out of range")
    {
        return std::format(
            "Number exceeds 32-bit integer limit. Max value is {}",
            type == TokenType::HEX_LITERAL ? "0x7FFFFFFF" : "2147483647");
    }

    return error; // "Unknown error";
}

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_integer_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    const auto reference_token = set.peek_next();

    switch (const auto type = reference_token.get_type())
    {
    case TokenType::INTEGER_LITERAL:
    case TokenType::LONG_INTEGER_LITERAL:
    case TokenType::HEX_LITERAL:
    {
        const std::string& input = reference_token.get_lexeme();
        set.skip(1);

        try
        {
            const long long int value = type == TokenType::HEX_LITERAL
                ? std::stoi(input, nullptr, 16)
                : type == TokenType::LONG_INTEGER_LITERAL
                ? std::stoll(input)
                : std::stoi(input);
            const short bit_count =
                reference_token.get_type() == TokenType::LONG_INTEGER_LITERAL
                ? 64
                : INFER_INT_BIT_COUNT(value);

            return std::make_unique<AstIntLiteral>(
                reference_token.get_source_fragment(),
                context,
                value,
                bit_count);
        }
        catch (const std::exception& e)
        {
            const auto reason = format_int_conversion_error(e.what(), type);
            throw parsing_error(
                ErrorType::SEMANTIC_ERROR,
                reason,
                reference_token.get_source_fragment());
        }
    }
    default:
        return std::nullopt;
    }
}

llvm::Value* AstIntLiteral::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    return llvm::ConstantInt::get(
        module->getContext(),
        llvm::APInt(this->bit_count(), this->value(), this->is_signed())
    );
}

std::unique_ptr<IAstNode> AstIntLiteral::clone()
{
    return std::make_unique<AstIntLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->value(),
        this->bit_count(),
        this->get_flags()
    );
}

std::string AstIntLiteral::to_string()
{
    return std::format("IntLiteral({} ({} bit))", value(), bit_count());
}
