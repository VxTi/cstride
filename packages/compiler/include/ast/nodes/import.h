#pragma once

#include <utility>

#include "ast/identifiers.h"
#include "ast/parsing_context.h"
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
            const std::shared_ptr<ParsingContext>& context,
            Dependency dependency
        ) : IAstNode(source, source_position, context),
            _dependency(std::move(dependency)) {}

        [[nodiscard]]
        const std::string& get_module() const { return this->_dependency.module_base; }

        [[nodiscard]]
        const Dependency& get_dependency() const { return this->_dependency; }

        [[nodiscard]]
        const std::vector<std::string>& get_submodules() const { return this->_dependency.submodules; }

        std::string to_string() override;
    };

    std::unique_ptr<AstImport> parse_import_statement(const std::shared_ptr<ParsingContext>& context, TokenSet& set);
}
