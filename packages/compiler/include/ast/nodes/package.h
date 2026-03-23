#pragma once

#include "ast_node.h"

namespace stride::ast
{
    class TokenSet;

    class AstPackage
        : public IAstNode
    {
        std::string _name;

    public:
        explicit AstPackage(
            const SourcePosition& source,
            std::string package_name
        ) :
            IAstNode(source),
            _name(std::move(package_name)) {}

        [[nodiscard]]
        const std::string& get_package_name() const
        {
            return this->_name;
        }

        void validate(const SymbolTable* symbol_table) override;

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override;
    };

    bool is_package_declaration(const TokenSet& set);

    std::unique_ptr<AstPackage> parse_package_declaration(
        TokenSet& set);
} // namespace stride::ast
