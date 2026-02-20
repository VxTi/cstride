#pragma once

#include "ast/nodes/ast_node.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"

#include <utility>

namespace stride::ast
{
    typedef struct Dependency
    {
        Symbol module_base;
        std::vector<Symbol> submodules;
    } Dependency;

    class AstImport : public IAstNode
    {
        const Dependency _dependency;

    public:
        explicit AstImport(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            Dependency dependency) :
            IAstNode(source, context),
            _dependency(std::move(dependency)) {}

        [[nodiscard]]
        const Symbol& get_module() const
        {
            return this->_dependency.module_base;
        }

        [[nodiscard]]
        const Dependency& get_dependency() const
        {
            return this->_dependency;
        }

        [[nodiscard]]
        const std::vector<Symbol>& get_submodules() const
        {
            return this->_dependency.submodules;
        }

        std::string to_string() override;
    };

    std::unique_ptr<AstImport> parse_import_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);
} // namespace stride::ast
