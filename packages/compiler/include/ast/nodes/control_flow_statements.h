#pragma once
#include "ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class IAstControlFlowStatement
        : public IAstNode
    {
    public:
        explicit IAstControlFlowStatement(const SourcePosition& source) :
            IAstNode(source) {}

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstContinueStatement
        : public IAstControlFlowStatement
    {
    public:
        explicit AstContinueStatement(const SourcePosition& source) :
            IAstControlFlowStatement(source) {}

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override;

        void validate(const SymbolTable* symbol_table) override;

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override
        {
            return "continue";
        }
    };

    class AstBreakStatement
        : public IAstControlFlowStatement
    {
    public:
        explicit AstBreakStatement(const SourcePosition& source) :
            IAstControlFlowStatement(source) {}

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override;

        void validate(const SymbolTable* symbol_table) override;

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override
        {
            return "break";
        }
    };

    std::unique_ptr<AstContinueStatement> parse_continue_statement(TokenSet& set);

    std::unique_ptr<AstBreakStatement> parse_break_statement(TokenSet& set);
}
