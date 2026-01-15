#pragma once

#include <sstream>
#include <utility>

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

using namespace std;

namespace stride::ast
{
    class AstImportNode : public AstNode
    {
        Symbol module;
        const vector<Symbol> dependencies;

    public:
        explicit AstImportNode(Symbol module, const Symbol& dependency) :
            module(std::move(module)),
            dependencies({dependency})
        {
        }

        explicit AstImportNode(Symbol module, const vector<Symbol>& dependencies) : module(std::move(module)),
            dependencies(dependencies)
        {
        }

        llvm::Value* codegen() override;

        std::string to_string() override;

        static bool can_parse(const TokenSet& tokens);

        static unique_ptr<AstImportNode> try_parse(Scope, TokenSet& tokens);
    };
}
