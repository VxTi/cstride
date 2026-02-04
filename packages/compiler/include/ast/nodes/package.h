#pragma once
#include "ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstPackage
        : public IAstNode
    {
        std::string _name;

    public:
        explicit AstPackage(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            std::string package_name
        ) : IAstNode(source, source_position, context),
            _name(std::move(package_name)) {};

        [[nodiscard]]
        const std::string& get_name() const { return this->_name; }

        void validate() override;

        std::string to_string() override;
    };

    bool is_package_declaration(const TokenSet& set);

    std::unique_ptr<AstPackage> parse_package_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );
}
