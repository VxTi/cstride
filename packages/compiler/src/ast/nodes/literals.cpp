#include "ast/nodes/literal_values.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    if (auto str = parse_string_literal_optional(context, set); str.
        has_value())
    {
        return str;
    }

    if (auto int_lit = parse_integer_literal_optional(context, set); int_lit.
        has_value())
    {
        return int_lit;
    }

    if (auto float_lit = parse_float_literal_optional(context, set);
        float_lit.has_value())
    {
        return float_lit;
    }

    if (auto char_lit = parse_char_literal_optional(context, set); char_lit.
        has_value())
    {
        return char_lit;
    }

    if (auto bool_lit = parse_boolean_literal_optional(context, set);
        bool_lit.has_value())
    {
        return bool_lit;
    }

    if (set.peek_next_eq(TokenType::KEYWORD_NIL))
    {
        const auto reference_token = set.next();
        return std::make_unique<AstNilLiteral>(
            reference_token.get_source_fragment(),
            context);
    }

    return std::nullopt;
}

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_string_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (const auto reference_token = set.peek_next();
        reference_token.get_type() == TokenType::STRING_LITERAL)
    {
        const auto str_tok = set.next();

        return std::make_unique<AstStringLiteral>(
            reference_token.get_source_fragment(),
            context,
            str_tok.get_lexeme()
        );
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_float_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (const auto reference_token = set.peek_next();
        reference_token.get_type() == TokenType::DOUBLE_LITERAL)
    {
        const auto next = set.next();
        const auto numeric = next.get_lexeme().substr(
            0,
            next.get_lexeme().length() - 1);
        // Remove the trailing D

        return std::make_unique<AstFpLiteral>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::FLOAT64,
            std::stod(numeric)
        );
    }
    if (const auto reference_token = set.peek_next();
        reference_token.get_type() == TokenType::FLOAT_LITERAL)
    {
        const auto next = set.next();
        return std::make_unique<AstFpLiteral>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::FLOAT32,
            std::stof(next.get_lexeme())
        );
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_boolean_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (const auto reference_token = set.peek_next();
        reference_token.get_type() == TokenType::BOOLEAN_LITERAL)
    {
        const auto next = set.next();
        const bool value = next.get_lexeme() == "true";

        return std::make_unique<AstBooleanLiteral>(
            reference_token.get_source_fragment(),
            context,
            value);
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_char_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (const auto reference_token = set.peek_next();
        reference_token.get_type() == TokenType::CHAR_LITERAL)
    {
        const auto next = set.next();
        const char value = next.get_lexeme()[0];

        return std::make_unique<AstCharLiteral>(
            reference_token.get_source_fragment(),
            context,
            value);
    }
    return std::nullopt;
}

std::string format_int_conversion_error(
    const std::string& error,
    const TokenType type
)
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
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();

    switch (const auto token_type = reference_token.get_type())
    {
    case TokenType::INTEGER_LITERAL:
    case TokenType::LONG_INTEGER_LITERAL:
    case TokenType::HEX_LITERAL:
    {
        const std::string& input = reference_token.get_lexeme();
        set.skip(1);

        try
        {
            const long long int value = token_type == TokenType::HEX_LITERAL
                ? std::stoi(input, nullptr, 16)
                : token_type == TokenType::LONG_INTEGER_LITERAL
                ? std::stoll(input)
                : std::stoi(input);

            const PrimitiveType type =
                reference_token.get_type() == TokenType::LONG_INTEGER_LITERAL
                ? PrimitiveType::INT64
                : PrimitiveType::INT32;

            return std::make_unique<AstIntLiteral>(reference_token.get_source_fragment(), context, type, value);
        }
        catch (const std::exception& e)
        {
            const auto reason = format_int_conversion_error(e.what(), token_type);
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

// --- codegen of literals

llvm::Value* AstCharLiteral::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return llvm::ConstantInt::get(
        module->getContext(),
        llvm::APInt(this->get_bit_count(), this->value(), true)
    );
}

llvm::Value* AstStringLiteral::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    // Check if a global variable already exists with the same value
    // If it does, we'll return a pointer to the existing global string
    // This prevents code duplication and improves performance by reusing existing strings
    for (auto& global : module->globals())
    {
        if (!global.hasInitializer())
        {
            continue;
        }

        if (const auto* const_entry =
            llvm::dyn_cast<llvm::ConstantDataArray>(global.getInitializer()))
        {
            if (const_entry->isCString() &&
                const_entry->getAsString().drop_back() == this->value())
            {
                // Return a pointer to the existing global string
                return builder->CreateInBoundsGEP(
                    global.getValueType(),
                    &global,
                    { builder->getInt32(0), builder->getInt32(0) });
            }
        }
    }

    return builder->CreateGlobalString(this->value(), "", 0, module);
}

llvm::Value* AstFpLiteral::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    if (this->get_primitive_type() == PrimitiveType::FLOAT64)
    {
        return llvm::ConstantFP::get(builder->getDoubleTy(), this->value());
    }
    return llvm::ConstantFP::get(builder->getFloatTy(), this->value());
}

llvm::Value* AstBooleanLiteral::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return llvm::ConstantInt::get(
        module->getContext(),
        llvm::APInt(this->get_bit_count(), this->value(), true)
    );
}

llvm::Value* AstNilLiteral::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return llvm::Constant::getNullValue(builder->getPtrTy());
}

llvm::Value* AstIntLiteral::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return llvm::ConstantInt::get(
        module->getContext(),
        llvm::APInt(this->get_bit_count(), this->value(), this->is_signed())
    );
}

// --- Cloning literal Ast nodes

std::unique_ptr<IAstNode> AstBooleanLiteral::clone()
{
    return std::make_unique<AstBooleanLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->value()
    );
}

std::unique_ptr<IAstNode> AstFpLiteral::clone()
{
    return std::make_unique<AstFpLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_primitive_type(),
        this->value()
    );
}

std::unique_ptr<IAstNode> AstStringLiteral::clone()
{
    return std::make_unique<AstStringLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->value()
    );
}

std::unique_ptr<IAstNode> AstCharLiteral::clone()
{
    return std::make_unique<AstCharLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->value()
    );
}

std::unique_ptr<IAstNode> AstIntLiteral::clone()
{
    return std::make_unique<AstIntLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_primitive_type(),
        this->value(),
        this->get_flags()
    );
}

std::unique_ptr<IAstNode> AstNilLiteral::clone()
{
    return std::make_unique<AstNilLiteral>(
        this->get_source_fragment(),
        this->get_context()
    );
}

// --- to_string methods

std::string AstCharLiteral::to_string()
{
    return std::format("CharLiteral({})", value());
}

std::string AstFpLiteral::to_string()
{
    return std::format("FpLiteral({} ({} bit))",
                       this->value(),
                       this->get_bit_count());
}

std::string AstBooleanLiteral::to_string()
{
    return std::format("BooleanLiteral({})", value());
}

std::string AstStringLiteral::to_string()
{
    return std::format("StringLiteral(\"{}\")", value());
}

std::string AstIntLiteral::to_string()
{
    return std::format("IntLiteral({} ({} bit))", value(), get_bit_count());
}
