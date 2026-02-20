#include "ast/nodes/literal_values.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>>
stride::ast::parse_char_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
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

std::string AstCharLiteral::to_string()
{
    return std::format("CharLiteral({})", value());
}

llvm::Value* AstCharLiteral::codegen(
    llvm::Module* module,
    llvm::IRBuilder<>* builder)
{
    return llvm::ConstantInt::get(
        module->getContext(),
        llvm::APInt(this->bit_count() * BITS_PER_BYTE, this->value(), true));
}
