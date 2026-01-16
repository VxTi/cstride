#pragma once
#include <utility>

#include "ast_node.h"
#include "primitive_type.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstFunctionParameter : IAstNode
    {
    public:
        const Symbol name;
        const std::unique_ptr<types::AstType> type;

        explicit AstFunctionParameter(
            Symbol param_name,
            std::unique_ptr<types::AstType> param_type
        ) :
            name(std::move(param_name)),
            type(std::move(param_type))
        {
        }

        std::string to_string() override;
    };

    std::unique_ptr<AstFunctionParameter> try_parse_first_fn_param(
        const Scope& scope,
        TokenSet& tokens
    );

    void try_parse_subsequent_fn_params(
        const Scope& scope,
        TokenSet& tokens,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );

    class AstFunctionDefinition :
        public IAstNode,
        public ISynthesisable
    {
        const std::unique_ptr<IAstNode> _body;
        const Symbol _name;
        const std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        const std::unique_ptr<types::AstType> _return_type;
        const bool _is_extern;
        const bool _is_variadic;

    public:
        AstFunctionDefinition(
            Symbol name,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<types::AstType> return_type,
            std::unique_ptr<IAstNode> body,
            const bool is_variadic = false,
            const bool is_extern = false
        )
            : _body(std::move(body)),
              _name(std::move(name)),
              _parameters(std::move(parameters)),
              _return_type(std::move(return_type)),
              _is_extern(is_extern),
              _is_variadic(is_variadic)
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        [[nodiscard]] Symbol name() const { return this->_name; }

        [[nodiscard]] IAstNode* body() const { return this->_body.get(); }

        [[nodiscard]] const std::vector<std::unique_ptr<AstFunctionParameter>>& parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]] const std::unique_ptr<types::AstType>& return_type() const { return this->_return_type; }

        [[nodiscard]] bool is_extern() const { return this->_is_extern; }

        [[nodiscard]] bool is_variadic() const { return this->_is_variadic; }

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstFunctionDefinition> try_parse(
            const Scope& scope,
            TokenSet& tokens
        );
    };
}
