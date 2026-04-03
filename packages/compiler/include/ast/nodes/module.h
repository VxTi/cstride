#pragma once

#include "ast_node.h"
#include "blocks.h"

#include <utility>

namespace stride::ast
{
    class AstModule
        : public IAstNode,
          public IAstContainer
    {
        std::string _name;
        std::unique_ptr<AstBlock> _body;

    public:
        std::string to_string() override;

        explicit AstModule(
            const SourcePosition& source,
            std::string name,
            std::unique_ptr<AstBlock> body
        ) :
            IAstNode(source),
            _name(std::move(name)),
            _body(std::move(body)) {}

        [[nodiscard]]
        const std::string& get_name() const
        {
            return _name;
        }

        [[nodiscard]]
        AstBlock* get_body() override
        {
            return this->_body.get();
        }

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module, llvm::IRBuilderBase* builder
        ) override;

        void resolve_forward_references(
            SymbolTable* symbol_table,
            llvm::Module* module, llvm::IRBuilderBase* builder) override;

        std::unique_ptr<IAstNode> clone() override;

        void validate(SymbolTable* symbol_table) override;
    };

    std::unique_ptr<AstModule> parse_module_statement(
        TokenSet& set);
} // namespace stride::ast
