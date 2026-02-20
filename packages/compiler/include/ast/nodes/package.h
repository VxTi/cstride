#pragma once
#include "ast/tokens/token_set.h"
#include "ast_node.h"

namespace stride::ast
{
    class AstPackage : public IAstNode
    {
        std::string _name;

    public:
        explicit AstPackage(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string package_name) :
            IAstNode(source, context),
            _name(std::move(package_name)) {};

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_name;
        }

        void validate() override;

        std::string to_string() override;
    };

    bool is_package_declaration(const TokenSet& set);

    std::unique_ptr<AstPackage> parse_package_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);
} // namespace stride::ast
