#include "ast/nodes/literal_values.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>>
stride::ast::parse_string_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    if (const auto reference_token = set.peek_next();
        reference_token.get_type() == TokenType::STRING_LITERAL)
    {
        const auto str_tok = set.next();

        return std::make_unique<AstStringLiteral>(
            reference_token.get_source_fragment(),
            context,
            str_tok.get_lexeme());
    }
    return std::nullopt;
}

llvm::Value* AstStringLiteral::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
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
            if (const_entry->isCString() && const_entry->getAsString().
                drop_back() == this->value())
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

std::unique_ptr<IAstExpression> AstStringLiteral::clone()
{
    return std::make_unique<AstStringLiteral>(
        this->get_source_fragment(),
        this->get_context(),
        this->value()
    );
}

std::string AstStringLiteral::to_string()
{
    return std::format("StringLiteral(\"{}\")", value());
}
