#include <llvm/IR/Constants.h>
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_float_literal_optional(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    if (const auto reference_token = set.peak_next(); reference_token.get_type() == TokenType::DOUBLE_LITERAL)
    {
        const auto next = set.next();
        const auto numeric = next.get_lexeme().substr(0, next.get_lexeme().length() - 1);
        // Remove the trailing D

        return std::make_unique<AstFpLiteral>(
            set.get_source(),
            reference_token.get_source_position(),
            scope,
            std::stod(numeric),
            64
        );
    }
    if (const auto reference_token = set.peak_next(); reference_token.get_type() == TokenType::FLOAT_LITERAL)
    {
        const auto next = set.next();
        return std::make_unique<AstFpLiteral>(
            set.get_source(),
            reference_token.get_source_position(),
            scope,
            std::stof(next.get_lexeme()),
            32
        );
    }
    return std::nullopt;
}

std::string AstFpLiteral::to_string()
{
    return std::format("FpLiteral({} ({} bit))", this->value(), this->bit_count());
}

llvm::Value* AstFpLiteral::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::IRBuilder<>* builder)
{
    if (this->bit_count() > 32)
    {
        return llvm::ConstantFP::get(builder->getDoubleTy(), this->value());
    }
    return llvm::ConstantFP::get(builder->getFloatTy(), this->value());
}
