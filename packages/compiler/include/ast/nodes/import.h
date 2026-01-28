#pragma once

#include "ast/identifiers.h"
#include "ast/symbol_registry.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    typedef struct Dependency
    {
        std::string module_base;
        std::vector<std::string> submodules;
    } Dependency;

    class AstImport
        : public IAstNode
    {
        const Dependency _dependency;

    public:
        explicit AstImport(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            const Dependency& dependency
        ) : IAstNode(source, source_position, scope),
            _dependency(dependency) {}

        [[nodiscard]]
        const std::string& get_module() const { return this->_dependency.module_base; }

        [[nodiscard]]
        const Dependency& get_dependency() const { return this->_dependency; }

        [[nodiscard]]
        const std::vector<std::string>& get_submodules() const { return this->_dependency.submodules; }

        std::string to_string() override;
    };

    bool is_import_statement(const TokenSet& tokens);

    std::unique_ptr<AstImport> parse_import_statement(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set);
}
