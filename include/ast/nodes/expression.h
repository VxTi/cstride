#pragma once

#include "ast_node.h"
#include "primitive_type.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define EXPRESSION_VARIABLE_DECLARATION        1
#define EXPRESSION_INLINE_VARIABLE_DECLARATION 2 // Variables declared after initial one
#define EXPRESSION_VARIABLE_ASSIGNATION        4

    class AstExpression :
        public virtual IAstNode,
        public virtual ISynthesisable
    {
    public:
        const std::vector<std::unique_ptr<IAstNode>> children;

        explicit AstExpression(std::vector<std::unique_ptr<IAstNode>> children) : children(std::move(children))
        {
        };

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;

        std::string to_string() override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstExpression> try_parse(const Scope& scope, TokenSet& tokens);

        static std::unique_ptr<AstExpression> try_parse_expression(
            int expression_type_flags,
            const Scope& scope,
            TokenSet& set
        );
    };

    class AstIdentifier : public AstExpression
    {
    public:
        const Symbol name;

        explicit AstIdentifier(Symbol name) :
            AstExpression({}),
            name(std::move(name))
        {
        }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;

        std::string to_string() override;
    };

    class AstFunctionInvocation :
        public AstExpression
    {
    public:
        const Symbol function_name;

        explicit AstFunctionInvocation(Symbol function_name) :
            AstExpression({}),
            function_name(std::move(function_name))
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;
    };

    class AstVariableDeclaration :
        public AstExpression
    {
        const Symbol variable_name;
        std::unique_ptr<types::AstType> variable_type;
        const std::unique_ptr<IAstNode> initial_value;

    public:
        AstVariableDeclaration(Symbol variable_name, std::unique_ptr<types::AstType> variable_type,
                               std::unique_ptr<IAstNode> initial_value) :
            AstExpression({}),
            variable_name(std::move(variable_name)),
            variable_type(std::move(variable_type)),
            initial_value(std::move(initial_value))
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;

        [[nodiscard]] const Symbol& get_variable_name() const
        {
            return this->variable_name;
        }

        [[nodiscard]] const types::AstType* get_variable_type() const
        {
            return this->variable_type.get();
        }

        [[nodiscard]] const IAstNode* get_initial_value() const
        {
            return this->initial_value.get();
        }
    };
}
