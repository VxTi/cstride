#include "ast/nodes/literal_values.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <sstream>

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
            reference_token.get_source_position(),
            context,
            str_tok.get_lexeme());
    }
    return std::nullopt;
}

std::string AstStringLiteral::to_string()
{
    return std::format("StringLiteral(\"{}\")", value());
}

llvm::Value* AstStringLiteral::codegen(
    const ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilder<>* ir_builder)
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
                return ir_builder->CreateInBoundsGEP(
                    global.getValueType(),
                    &global,
                    { ir_builder->getInt32(0), ir_builder->getInt32(0) });
            }
        }
    }

    return ir_builder->CreateGlobalString(this->value(), "", 0, module);
}
