#pragma once

#include "ast_node.h"
#include "types.h"

#include <format>
#include <utility>

namespace stride:: ast
{
    enum class VisibilityModifier;
    class TokenSet;

    class AstTypeDefinition
        : public IAstNode
    {
        std::string _name;
        std::unique_ptr<IAstType> _type;
        VisibilityModifier _visibility;

    public:
        explicit AstTypeDefinition(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string  name,
            std::unique_ptr<IAstType> type,
            const VisibilityModifier visibility
        ) :
            IAstNode(source, context),
            _name(std::move(name)),
            _type(std::move(type)),
            _visibility(visibility) {}

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

        [[nodiscard]]
        const VisibilityModifier& get_visibility() const
        {
            return this->_visibility;
        }

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilder<>* builder) override;

        void resolve_forward_references(const ParsingContext* context, llvm::Module* module, llvm::IRBuilder<>* builder) override;

        std::string to_string() override { return std::format("Type<{}>", this->get_name()); }
    };

    std::unique_ptr<AstTypeDefinition> parse_type_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        VisibilityModifier modifier
    );
}
