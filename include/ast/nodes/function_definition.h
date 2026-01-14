#pragma once
#include <utility>

#include "ast_node.h"
#include "type.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstFunctionParameterNode : AstNode
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

        llvm::Value* codegen() override;

        static std::unique_ptr<AstFunctionParameterNode> try_parse(
        const Scope& scope,
            const std::unique_ptr<TokenSet>& tokens);
    };

    class AstFunctionDefinitionNode : public AstNode
    {
    private:
        const std::unique_ptr<AstNode> body_;
        const Symbol name_;
        const std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters_;
        const std::unique_ptr<AstType> return_type_;

    public:
        AstFunctionDefinitionNode(
            Symbol name,
            std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters,
            std::unique_ptr<AstType> return_type,
            std::unique_ptr<AstNode> body
        )
            : body_(std::move(body)),
              name_(std::move(name)),
              parameters_(std::move(parameters)),
              return_type_(std::move(return_type))
        {
        }

        std::string to_string() override;

        llvm::Value* codegen() override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstFunctionDefinitionNode> try_parse(
        const Scope& scope,
            const std::unique_ptr<TokenSet>& tokens);
    };
}
