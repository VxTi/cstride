#pragma once
#include <utility>

#include "ast_node.h"
#include "primitive_type.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstFunctionParameterNode : IAstNode
    {
    public:
        const Symbol name;
        const std::unique_ptr<types::AstType> type;

        explicit AstFunctionParameterNode(
            Symbol param_name,
            std::unique_ptr<types::AstType> param_type
        ) :
            name(std::move(param_name)),
            type(std::move(param_type))
        {
        }

        std::string to_string() override;

        static std::unique_ptr<AstFunctionParameterNode> try_parse(
            const Scope& scope,
            TokenSet& tokens
        );

        static void try_parse_subsequent_parameters(
            const Scope& scope,
            TokenSet& tokens,
            std::vector<std::unique_ptr<AstFunctionParameterNode>>& parameters
        );
    };

    class AstFunctionDefinitionNode :
        public IAstNode,
        public ISynthesisable
    {
        const std::unique_ptr<IAstNode> _body;
        const Symbol _name;
        const std::vector<std::unique_ptr<AstFunctionParameterNode>> _parameters;
        const std::unique_ptr<types::AstType> _return_type;
        const bool _is_extern;

    public:
        AstFunctionDefinitionNode(
            Symbol name,
            std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters,
            std::unique_ptr<types::AstType> return_type,
            std::unique_ptr<IAstNode> body,
            bool is_extern = false
        )
            : _body(std::move(body)),
              _name(std::move(name)),
              _parameters(std::move(parameters)),
              _return_type(std::move(return_type)),
              _is_extern(is_extern)
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        [[nodiscard]] Symbol name() const { return this->_name; }

        [[nodiscard]] IAstNode* body() const { return this->_body.get(); }

        [[nodiscard]] const std::vector<std::unique_ptr<AstFunctionParameterNode>>& parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]] const std::unique_ptr<types::AstType>& return_type() const { return this->_return_type; }

        [[nodiscard]] bool is_extern() const { return this->_is_extern; }

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstFunctionDefinitionNode> try_parse(
            const Scope& scope,
            TokenSet& tokens
        );
    };
}
