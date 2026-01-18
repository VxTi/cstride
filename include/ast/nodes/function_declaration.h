#pragma once
#include <utility>

#include "ast_node.h"
#include "primitive_type.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define SRFLAG_FN_DEF_EXTERN   (0x1)
#define SRFLAG_FN_DEF_VARIADIC (0x2)
#define SRFLAG_FN_DEF_MUTABLE  (0x3)

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function parameter definitions               *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionParameter : IAstNode
    {
    public:
        const Symbol name;
        const std::unique_ptr<types::AstType> type;

        explicit AstFunctionParameter(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol param_name,
            std::unique_ptr<types::AstType> param_type
        ) :
            IAstNode(source, source_offset),
            name(std::move(param_name)),
            type(std::move(param_type)) {}

        std::string to_string() override;
    };

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  *
     *                                                             *
     *                Function declaration definitions             *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionDeclaration :
        public IAstNode,
        public ISynthesisable
    {
        std::unique_ptr<IAstNode> _body;
        Symbol _name;
        std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        std::unique_ptr<types::AstType> _return_type;
        int _flags;

    public:
        AstFunctionDeclaration(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol name,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<IAstNode> body,
            std::unique_ptr<types::AstType> return_type,
            const int flags
        ) :
            IAstNode(source, source_offset),
            _body(std::move(body)),
            _name(std::move(name)),
            _parameters(std::move(parameters)),
            _return_type(std::move(return_type)),
            _flags(flags) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        [[nodiscard]]
        Symbol name() const { return this->_name; }

        [[nodiscard]]
        IAstNode* body() const { return this->_body.get(); }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstFunctionParameter>>& parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]]
        const std::unique_ptr<types::AstType>& return_type() const { return this->_return_type; }

        [[nodiscard]]
        bool is_extern() const { return this->_flags & SRFLAG_FN_DEF_EXTERN; }

        [[nodiscard]]
        bool is_variadic() const { return this->_flags & SRFLAG_FN_DEF_VARIADIC; }

        [[nodiscard]]
        bool is_mutable() const { return this->_flags & SRFLAG_FN_DEF_MUTABLE; }
    };

    bool is_fn_declaration(const TokenSet& tokens);

    std::unique_ptr<AstFunctionDeclaration> parse_fn_declaration(
        const Scope& scope,
        TokenSet& tokens
    );

    std::unique_ptr<AstFunctionParameter> parse_first_fn_param(
        const Scope& scope,
        TokenSet& tokens
    );

    void parse_subsequent_fn_params(
        const Scope& scope,
        TokenSet& tokens,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );

    std::unique_ptr<AstFunctionParameter> parse_variadic_fn_param(
        const Scope& scope,
        TokenSet& tokens
    );
}
