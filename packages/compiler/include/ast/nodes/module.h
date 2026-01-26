#pragma once
#include <utility>

#include "ast_node.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstModule : public IAstNode
    {
        std::string _name;

    public:
        std::string to_string() override;

        explicit AstModule(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::string name
        )
            : IAstNode(source, source_offset, scope),
              _name(std::move(name)) {}

        [[nodiscard]]
        const std::string& get_name() const { return _name; }
    };

    bool is_module_statement(const TokenSet& tokens);

    std::unique_ptr<AstModule> parse_module_statement(
        const std::shared_ptr<SymbolRegistry>&,
        TokenSet& tokens
    );
}
