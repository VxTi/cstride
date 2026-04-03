#pragma once

#include "ast_node.h"
#include "ast/symbol_table.h"
#include "ast/tokens/token_set.h"

#include <utility>

namespace stride::ast
{
    // TODO: Implement
    class AstSwitchBranch
        : public IAstNode
    {
    public:
        explicit AstSwitchBranch(const SourcePosition& source) :
            IAstNode(source) {}
    };

    class AstSwitch
        : public IAstNode
    {
        std::string _name;
        std::vector<std::unique_ptr<AstSwitchBranch>> _branches;

    public:
        explicit AstSwitch(
            const SourcePosition& source,
            std::string name
        ) :
            IAstNode(source),
            _name(std::move(name)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;
    };
} // namespace stride::ast
