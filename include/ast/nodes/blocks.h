#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstBlockNode :
        public virtual IAstNode,
        public virtual ISynthesisable
    {
        std::vector<std::unique_ptr<IAstNode>> children;

    public:
        explicit AstBlockNode(
            std::vector<std::unique_ptr<IAstNode>> children
        ) : children(std::move(children))
        {
        };

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;

        static std::optional<TokenSet> collect_block(TokenSet& set);

        static std::unique_ptr<IAstNode> try_parse(const Scope& scope, TokenSet& set);

        static std::unique_ptr<IAstNode> try_parse_block(const Scope& scope, TokenSet& set);
    };
}
