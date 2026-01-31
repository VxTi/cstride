#pragma once
#include <utility>

#include "ast_node.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    // TODO: Implement
    class AstSwitchBranch
        : public IAstNode
    {
    public:
        AstSwitchBranch(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry
        )
            : IAstNode(source, source_position, registry) {}
    };

    class AstSwitch :
        public IAstNode,
        public ISynthesisable
    {
        std::string _name;
        std::vector<std::unique_ptr<AstSwitchBranch>> _branches;

    public:
        explicit AstSwitch(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::string name
        )
            : IAstNode(source, source_position, registry),
              _name(std::move(name)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstSwitch> try_parse(const SymbolRegistry& registry, TokenSet& set);
    };
}
