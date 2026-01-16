#pragma once

#include <sstream>
#include <utility>

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstImport : public IAstNode
    {
        Symbol module;
        const std::vector<Symbol> dependencies;

    public:
        explicit AstImport(Symbol module, const Symbol& dependency) :
            module(std::move(module)),
            dependencies({dependency})
        {
        }

        explicit AstImport(Symbol module, const std::vector<Symbol>& dependencies) : module(std::move(module)),
            dependencies(dependencies)
        {
        }

        std::string to_string() override;
    };

    bool is_import_statement(const TokenSet& tokens);

    std::unique_ptr<AstImport> parse_import_statement(const Scope& scope, TokenSet& tokens);
}
