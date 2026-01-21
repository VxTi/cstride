#include "ast/nodes/literal_values.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_float_literal_optional(
    [[maybe_unused]] const Scope& scope,
    TokenSet& set
)
{
    if (const auto reference_token = set.peak_next(); reference_token.type == TokenType::DOUBLE_LITERAL)
    {
        const auto next = set.next();
        return std::make_unique<AstFpLiteral>(
            set.source(),
            reference_token.offset,
            std::stod(next.lexeme),
            8
        );
    }
    if (const auto reference_token = set.peak_next(); reference_token.type == TokenType::FLOAT_LITERAL)
    {
        const auto next = set.next();
        return std::make_unique<AstFpLiteral>(
            set.source(),
            reference_token.offset,
            std::stof(next.lexeme),
            4
        );
    }
    return std::nullopt;
}

std::string AstFpLiteral::to_string()
{
    return std::format("FfLiteral({} ({}bit))", this->value(), this->bit_count());
}

llvm::Value* AstFpLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    if (this->bit_count() > 32)
    {
        return llvm::ConstantFP::get(builder->getDoubleTy(), this->value());
    }
    return llvm::ConstantFP::get(builder->getFloatTy(), this->value());
}
