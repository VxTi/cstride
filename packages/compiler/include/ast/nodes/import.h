#pragma once

#include "expression.h"
#include "ast/symbols.h"
#include "ast/nodes/ast_node.h"

#include <utility>

namespace stride::ast
{
    class TokenSet;
    class SymbolTable;

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
            const SourcePosition& source,
            std::unique_ptr<AstIdentifier> package_identifier,
            std::vector<std::unique_ptr<AstIdentifier>> import_list
        ) :
            IAstNode(source),
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

        void validate(SymbolTable* symbol_table) override;

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override;
    };

    std::unique_ptr<AstImport> parse_import_statement(
        TokenSet& set
    );
} // namespace stride::ast
