#pragma once
#include "ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class IControlFlowStatement
        : public IAstNode
    {
    public:
        explicit IControlFlowStatement(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context
        ) :
            IAstNode(source, context) {}
    };

    class AstContinueStatement
        : public IControlFlowStatement
    {
    public:
        explicit AstContinueStatement(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context
        ) :
            IControlFlowStatement(source, context) {}

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override;

        void validate() override;

        std::string to_string() override { return "continue"; }
    };

    class AstBreakStatement
        : public IControlFlowStatement
    {
    public:
        explicit AstBreakStatement(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context
        ) :
            IControlFlowStatement(source, context) {}

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override;

        void validate() override;

        std::string to_string() override { return "break"; }
    };

    std::unique_ptr<AstContinueStatement> parse_continue_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    std::unique_ptr<AstBreakStatement> parse_break_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );
}
