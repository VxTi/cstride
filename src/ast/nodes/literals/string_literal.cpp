#include "ast/nodes/literals.h"
#include <llvm/IR/IRBuilder.h>
#include <sstream>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> AstStringLiteral::try_parse_optional(
    [[maybe_unused]] const Scope& scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::STRING_LITERAL))
    {
        const auto str_tok = tokens.next();

        return std::make_unique<AstStringLiteral>(str_tok.lexeme);
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
