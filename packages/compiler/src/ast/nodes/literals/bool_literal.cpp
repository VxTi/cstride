#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>

#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_boolean_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (const auto reference_token = set.peek_next(); reference_token.get_type() == TokenType::BOOLEAN_LITERAL)
    {
        const auto next = set.next();
        const bool value = next.get_lexeme() == "true";

        return std::make_unique<AstBooleanLiteral>(
            reference_token.get_source_position(),
            context,
            value
        );
    }
    return std::nullopt;
}

std::string AstBooleanLiteral::to_string()
{
    return std::format("BooleanLiteral({})", value());
}

llvm::Value* AstBooleanLiteral::codegen(
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
            true
        )
    );
}
