#include "ast/nodes/literal_values.h"

#include <sstream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_string_literal_optional(
    [[maybe_unused]] const Scope& scope,
    TokenSet& set
)
{
    if (const auto reference_token = set.peak_next(); reference_token.type == TokenType::STRING_LITERAL)
    {
        const auto str_tok = set.next();

        return std::make_unique<AstStringLiteral>(
            set.source(),
            reference_token.offset,
            str_tok.lexeme
        );
    }
    return std::nullopt;
}

std::string AstStringLiteral::to_string()
{
    return std::format("StringLiteral(\"{}\")", value());
}

llvm::Value* AstStringLiteral::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* irBuilder)
{
    // Check if a global variable already exists with the same value
    // If it does, we'll return a pointer to the existing global string
    // This prevents code duplication and improves performance by reusing existing strings
    for (auto& global : module->globals())
    {
        if (!global.hasInitializer()) { continue; }

        if (const auto* constData = llvm::dyn_cast<llvm::ConstantDataArray>(global.getInitializer()))
        {
            if (constData->isCString() && constData->getAsString().drop_back() == this->value())
            {
                // Return a pointer to the existing global string
                return irBuilder->CreateInBoundsGEP(
                    global.getValueType(),
                    &global,
                    {irBuilder->getInt32(0), irBuilder->getInt32(0)}
                );
            }
        }
    }

    return irBuilder->CreateGlobalString(this->value());
}
