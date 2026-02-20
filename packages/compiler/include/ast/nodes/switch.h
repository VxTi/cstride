#pragma once
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"
#include "ast_node.h"

#include <utility>

namespace stride::ast
{
    // TODO: Implement
    class AstSwitchBranch : public IAstNode
    {
    public:
        AstSwitchBranch(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context) :
            IAstNode(source, context) {}
    };

    class AstSwitch : public IAstNode, public ISynthesisable
    {
        std::string _name;
        std::vector<std::unique_ptr<AstSwitchBranch>> _branches;

    public:
        explicit AstSwitch(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string name) :
            IAstNode(source, context),
            _name(std::move(name)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstSwitch> try_parse(
            const ParsingContext& context,
            TokenSet& set);
    };
} // namespace stride::ast
