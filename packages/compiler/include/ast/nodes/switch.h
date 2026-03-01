#pragma once

#include "ast_node.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"

#include <utility>

namespace stride::ast
{
    // TODO: Implement
    class AstSwitchBranch
        : public IAstNode
    {
    public:
        AstSwitchBranch(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context) :
            IAstNode(source, context) {}
    };

    class AstSwitch
        : public IAstNode
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
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;
    };
} // namespace stride::ast
