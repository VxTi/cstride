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
        explicit AstImport(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol module,
            const std::vector<Symbol>& dependencies
        ) : IAstNode(source, source_offset),
            module(std::move(module)),
            dependencies(dependencies) {}

        explicit AstImport(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol module,
            const Symbol& dependency
        ) :
            AstImport(
                source,
                source_offset,
                std::move(module),
                std::vector{dependency}
            ) {}

        std::string to_string() override;
    };

    bool is_import_statement(const TokenSet& tokens);

    std::unique_ptr<AstImport> parse_import_statement(const Scope& scope, TokenSet& set);
}
