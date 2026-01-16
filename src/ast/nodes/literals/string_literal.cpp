#include "ast/nodes/literals.h"
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> AstStringLiteral::try_parse_optional(
    [[maybe_unused]] const Scope& scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::STRING_LITERAL))
    {
        const auto sym = tokens.next();

        const auto concatenated = sym.lexeme.substr(1, sym.lexeme.size() - 2);

        return std::make_unique<AstStringLiteral>(concatenated);
    }
    return std::nullopt;
}

std::string AstStringLiteral::to_string()
{
    return std::format("StringLiteral({})", value());
}

llvm::Value* AstStringLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irBuilder)
{
    return irBuilder->CreateGlobalString(_value);
}