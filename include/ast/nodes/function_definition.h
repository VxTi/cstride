#pragma once
#include <utility>

#include "ast_node.h"
#include "types.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define SRFLAG_FN_DEF_EXTERN   (0x1)
#define SRFLAG_FN_DEF_VARIADIC (0x2)
#define SRFLAG_FN_DEF_MUTABLE  (0x3)

#define SRFLAG_FN_PARAM_DEF_VARIADIC (0x1)
#define SRFLAG_FN_PARAM_DEF_MUTABLE  (0x2)

#define MAX_FUNCTION_PARAMETERS      (32)

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function parameter definitions               *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionParameter : IAstNode
    {
        const std::string _name;
        std::shared_ptr<IAstInternalFieldType> _type;
        const int _flags;

    public:
        explicit AstFunctionParameter(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string param_name,
            const std::shared_ptr<IAstInternalFieldType> &param_type,
            const int flags
        ) :
            IAstNode(source, source_offset),
            _name(std::move(param_name)),
            _type(std::move(param_type)),
            _flags(flags) {}

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return this->_name; }

        [[nodiscard]]
        std::shared_ptr<IAstInternalFieldType> get_type() const { return this->_type; }
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
        std::string _name;
        std::string _internal_name;
        std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        std::unique_ptr<IAstInternalFieldType> _return_type;
        int _flags;

    public:
        AstFunctionDeclaration(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string name,
            std::string internal_name,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<IAstNode> body,
            std::unique_ptr<IAstInternalFieldType> return_type,
            const int flags
        ) :
            IAstNode(source, source_offset),
            _body(std::move(body)),
            _name(std::move(name)),
            _internal_name(std::move(internal_name)),
            _parameters(std::move(parameters)),
            _return_type(std::move(return_type)),
            _flags(flags) {}

        std::string to_string() override;

        void define_symbols(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        [[nodiscard]]
        std::string get_name() const { return this->_name; }

        [[nodiscard]]
        std::string get_internal_name() const { return this->_internal_name; }

        [[nodiscard]]
        IAstNode* body() const { return this->_body.get(); }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstFunctionParameter>>& get_parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]]
        const std::unique_ptr<IAstInternalFieldType>& return_type() const { return this->_return_type; }

        [[nodiscard]]
        bool is_extern() const { return this->_flags & SRFLAG_FN_DEF_EXTERN; }

        [[nodiscard]]
        bool is_variadic() const { return this->_flags & SRFLAG_FN_DEF_VARIADIC; }

        [[nodiscard]]
        bool is_mutable() const { return this->_flags & SRFLAG_FN_DEF_MUTABLE; }

    private:
        std::optional<std::vector<llvm::Type*>> resolve_parameter_types(
            llvm::Module* module,
            llvm::LLVMContext& context
        ) const;
    };

    bool is_fn_declaration(const TokenSet& tokens);

    std::unique_ptr<AstFunctionDeclaration> parse_fn_declaration(
        const std::shared_ptr<Scope>& scope,
        TokenSet& tokens
    );

    std::unique_ptr<AstFunctionParameter> parse_standalone_fn_param(
        std::shared_ptr<Scope> scope,
        TokenSet& set
    );

    void parse_subsequent_fn_params(
        const std::shared_ptr<Scope>& scope,
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );

    void parse_variadic_fn_param(
        const std::shared_ptr<Scope>& scope,
        TokenSet& tokens,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );
}
