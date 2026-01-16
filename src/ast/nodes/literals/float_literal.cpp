#include "ast/nodes/literals.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> AstFloatLiteral::try_parse_optional(
    const Scope& scope,
    TokenSet& tokens
)
{
    if (tokens.peak_next_eq(TokenType::FLOAT_LITERAL))
    {
        const auto next = tokens.next();
        return std::make_unique<AstFloatLiteral>(std::stof(next.lexeme));
    }
    return std::nullopt;
}

std::string AstFloatLiteral::to_string()
{
    return std::format("FloatLiteral({})", value());
}

llvm::Value* AstFloatLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    return llvm::ConstantFP::get(context, llvm::APFloat(_value));
}