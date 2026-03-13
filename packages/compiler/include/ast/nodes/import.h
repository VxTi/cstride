#pragma once

#include "expression.h"
#include "ast/symbols.h"
#include "ast/nodes/ast_node.h"

#include <utility>

namespace stride::ast
{
    class TokenSet;
    class ParsingContext;

    typedef struct Dependency
    {
        Symbol package_name;
        std::vector<Symbol> submodules;
    } Dependency;

    class AstImport
        : public IAstNode
    {

        std::unique_ptr<AstIdentifier> _package_identifier;
        std::vector<std::unique_ptr<AstIdentifier>> _import_list;

    public:
        explicit AstImport(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstIdentifier> package_identifier,
            std::vector<std::unique_ptr<AstIdentifier>> import_list
        ) :
            IAstNode(source, context),
            _package_identifier(std::move(package_identifier)),
            _import_list(std::move(import_list)) {}

        [[nodiscard]]
        AstIdentifier* get_package_identifier() const
        {
            return this->_package_identifier.get();
        }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstIdentifier>>& get_import_list() const
        {
            return this->_import_list;
        }

        void validate() override;

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override;
    };

    std::unique_ptr<AstImport> parse_import_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );
} // namespace stride::ast
