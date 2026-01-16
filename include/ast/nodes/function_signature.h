#pragma once
#include <utility>

#include "ast_node.h"
#include "type.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstFunctionParameterNode : IAstNode
    {
    public:
        const Symbol name;
        const std::unique_ptr<AstType> type;

        explicit AstFunctionParameterNode(
            Symbol param_name,
            std::unique_ptr<AstType> param_type
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
        const std::unique_ptr<AstType> _return_type;

    public:
        AstFunctionDefinitionNode(
            Symbol name,
            std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters,
            std::unique_ptr<AstType> return_type,
            std::unique_ptr<IAstNode> body
        )
            : _body(std::move(body)),
              _name(std::move(name)),
              _parameters(std::move(parameters)),
              _return_type(std::move(return_type))
        {
        }

        std::string to_string() override;

        llvm::Value* codegen() override;

        [[nodiscard]] Symbol name() const { return this->_name; }

        [[nodiscard]] IAstNode* body() const { return this->_body.get(); }

        [[nodiscard]] const std::vector<std::unique_ptr<AstFunctionParameterNode>>& parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]] const std::unique_ptr<AstType>& return_type() const { return this->_return_type; }

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstFunctionDefinitionNode> try_parse(
            const Scope& scope,
            TokenSet& tokens
        );
    };
}
