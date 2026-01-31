#pragma once
#include <utility>

#include "ast_node.h"
#include "blocks.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstModule
        : public IAstNode,
          public ISynthesisable
    {
        std::string _name;
        std::unique_ptr<AstBlock> _body;

    public:
        std::string to_string() override;

        explicit AstModule(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::string name,
            std::unique_ptr<AstBlock> body
        )
            : IAstNode(source, source_position, registry),
              _name(std::move(name)),
              _body(std::move(body)) {}

        [[nodiscard]]
        const std::string& get_name() const { return _name; }

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        void resolve_forward_references(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;
    };

    bool is_module_statement(const TokenSet& tokens);

    std::unique_ptr<AstModule> parse_module_statement(
        const std::shared_ptr<SymbolRegistry>&,
        TokenSet& set
    );
}
