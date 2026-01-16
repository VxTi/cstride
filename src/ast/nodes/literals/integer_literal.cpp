#include "ast/nodes/literals.h"
#include <llvm/IR/Constants.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> AstIntegerLiteral::try_parse_optional(const Scope& scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::INTEGER_LITERAL))
    {
        const auto next = tokens.next();

        return std::make_unique<AstIntegerLiteral>(
            std::move(std::stoi(next.lexeme))
        );
    }
    return std::nullopt;
}

std::string AstIntegerLiteral::to_string()
{
    return std::format("IntLiteral({})", value());
}

llvm::Value* AstIntegerLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    // 32-bit integer
    return llvm::ConstantInt::get(context, llvm::APInt(32, _value, true));
}
