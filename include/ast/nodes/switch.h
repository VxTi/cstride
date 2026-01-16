#pragma once
#include "ast_node.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstSwitchBranch : public IAstNode
    {

    };

    class AstSwitch :
        public IAstNode,
        public ISynthesisable
    {
        Symbol _name;
        std::vector<std::unique_ptr<AstSwitchBranch>> _branches;

    public:
        explicit AstSwitch(
            const Symbol& name
        ) : _name(name)
        {
        }

        std::string to_string() override;

        llvm::Value* codegen() override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstSwitch> try_parse(const Scope& scope, TokenSet& tokens);
    };
}
