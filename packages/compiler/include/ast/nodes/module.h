#pragma once
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"
#include "ast_node.h"
#include "blocks.h"

#include <utility>

namespace stride::ast
{
    class AstModule : public IAstNode, public ISynthesisable
    {
        std::string _name;
        std::unique_ptr<AstBlock> _body;

    public:
        std::string to_string() override;

        explicit AstModule(
            const SourceLocation& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string name,
            std::unique_ptr<AstBlock> body) :
            IAstNode(source, context),
            _name(std::move(name)),
            _body(std::move(body)) {}

        [[nodiscard]]
        const std::string& get_name() const
        {
            return _name;
        }

        [[nodiscard]]
        AstBlock* get_body() const
        {
            return this->_body.get();
        }

        llvm::Value* codegen(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    std::unique_ptr<AstModule> parse_module_statement(
        const std::shared_ptr<ParsingContext>&,
        TokenSet& set);
} // namespace stride::ast
