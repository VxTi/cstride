#pragma once

#include "ast_node.h"
#include "types.h"

#include <format>

namespace stride:: ast
{
    class TokenSet;

    class AstTypeDefinition
        : public IAstNode
    {
        std::string _name;
        std::unique_ptr<IAstType> _type;

    public:
        explicit AstTypeDefinition(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const std::string& name,
            std::unique_ptr<IAstType> type
        ) :
            IAstNode(source, context),
            _name(name),
            _type(std::move(type)) {}

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        const IAstType* get_type() const
        {
            return this->_type.get();
        }

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilder<>* builder) override { return nullptr; }

        std::string to_string() override { return std::format("Type {}<{}>", this->_name, this->_reference_name); }
    };

    std::unique_ptr<AstTypeDefinition> parse_type_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );
}
