#pragma once
#include <utility>

#include "ast_node.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstSwitchBranch : public IAstNode {};

    class AstSwitch :
        public IAstNode,
        public ISynthesisable
    {
        std::string _name;
        std::vector<std::unique_ptr<AstSwitchBranch>> _branches;

    public:
        explicit AstSwitch(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::string  name
        )
            : IAstNode(source, source_offset, scope),
              _name(std::move(name)) {}

        std::string to_string() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstSwitch> try_parse(const SymbolRegistry& scope, TokenSet& set);
    };
}
