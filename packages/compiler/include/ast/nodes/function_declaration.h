#pragma once
#include <utility>

#include "ast_node.h"
#include "blocks.h"
#include "types.h"
#include "ast/modifiers.h"
#include "ast/context.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define MAX_FUNCTION_PARAMETERS      (32)

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function parameter definitions               *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionParameter : public IAstNode
    {
        const std::string _name;
        std::unique_ptr<IAstType> _type;

    public:
        explicit AstFunctionParameter(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            std::string param_name,
            std::unique_ptr<IAstType> param_type
        ) : IAstNode(source, source_position, context),
            _name(std::move(param_name)),
            _type(std::move(param_type)) {}

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return this->_name; }

        [[nodiscard]]
        IAstType* get_type() const { return this->_type.get(); }

        ~AstFunctionParameter() override = default;
    };

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function declaration definitions             *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionDeclaration :
        public IAstNode,
        public ISynthesisable,
        public IAstContainer
    {
        std::unique_ptr<AstBlock> _body;
        Symbol _symbol;
        std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        std::shared_ptr<IAstType> _return_type;
        int _flags;

    public:
        AstFunctionDeclaration(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::shared_ptr<IAstType> return_type,
            const int flags
        ) :
            IAstNode(source, source_position, context),
            _body(std::move(body)),
            _symbol(std::move(symbol)),
            _parameters(std::move(parameters)),
            _return_type(std::move(return_type)),
            _flags(flags) {}

        std::string to_string() override;

        void resolve_forward_references(
            const std::shared_ptr<ParsingContext>& context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        llvm::Value* codegen(
            const std::shared_ptr<ParsingContext>& context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        [[nodiscard]]
        std::string get_name() const { return this->_symbol.name; }

        [[nodiscard]]
        std::string get_internal_name() const { return this->_symbol.internal_name; }

        [[nodiscard]]
        AstBlock* get_body() override { return this->_body.get(); }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstFunctionParameter>>& get_parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]]
        IAstType* get_return_type() const { return this->_return_type.get(); }

        [[nodiscard]]
        bool is_extern() const { return this->_flags & SRFLAG_FN_DEF_EXTERN; }

        [[nodiscard]]
        bool is_variadic() const { return this->_flags & SRFLAG_FN_DEF_VARIADIC; }

        [[nodiscard]]
        bool is_mutable() const { return this->_flags & SRFLAG_FN_DEF_MUTABLE; }

        void validate() override;

        ~AstFunctionDeclaration() override = default;

    private:
        std::optional<std::vector<llvm::Type*>> resolve_parameter_types(llvm::Module* module) const;
    };

    std::unique_ptr<AstFunctionDeclaration> parse_fn_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& tokens,
        VisibilityModifier modifier
    );

    std::unique_ptr<AstFunctionParameter> parse_standalone_fn_param(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    void parse_subsequent_fn_params(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );

    void parse_variadic_fn_param(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& tokens,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );
}
