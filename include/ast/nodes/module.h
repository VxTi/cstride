#pragma once
#include "ast_node.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstModule : public IAstNode
    {
        Symbol _name;

    public:
        std::string to_string() override;

        explicit AstModule(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol name
        )
            : IAstNode(source, source_offset),
              _name(std::move(name)) {}

        [[nodiscard]]
        Symbol name() const { return _name; }
    };

    bool is_module_statement(const TokenSet& tokens);

    std::unique_ptr<AstModule> parse_module_statement(const Scope& scope, TokenSet& tokens);
}
