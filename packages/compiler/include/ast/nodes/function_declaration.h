#pragma once
#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"
#include "ast_node.h"
#include "blocks.h"
#include "expression.h"
#include "types.h"

#include <utility>

namespace stride::ast
{
#define MAX_FUNCTION_PARAMETERS (32)

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function parameter definitions               *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionParameter
        : public IAstNode
    {
        const std::string _name;
        std::unique_ptr<IAstType> _type;

    public:
        explicit AstFunctionParameter(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string param_name,
            std::unique_ptr<IAstType> param_type
        ) :
            IAstNode(source, context),
            _name(std::move(param_name)),
            _type(std::move(param_type)) {}

        std::string to_string() override;

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        IAstType* get_type() const
        {
            return this->_type.get();
        }

        ~AstFunctionParameter() override = default;
    };

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function declaration definitions             *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class IAstCallable : public IAstContainer, public AstExpression
    {
        std::unique_ptr<AstBlock> _body;
        Symbol _symbol;
        std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        std::shared_ptr<IAstType> _return_type;
        int _flags;

    public:
        explicit IAstCallable(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::shared_ptr<IAstType> return_type,
            const int flags) :
            AstExpression(source, context),
            _body(std::move(body)),
            _symbol(std::move(symbol)),
            _parameters(std::move(parameters)),
            _return_type(std::move(return_type)),
            _flags(flags) {}


        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_symbol.name;
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_symbol.internal_name;
        }

        [[nodiscard]]
        AstBlock* get_body() override
        {
            return this->_body.get();
        }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstFunctionParameter>>&
        get_parameters() const
        {
            return this->_parameters;
        }

        [[nodiscard]]
        IAstType* get_return_type() const
        {
            return this->_return_type.get();
        }

        [[nodiscard]]
        bool is_extern() const
        {
            return this->_flags & SRFLAG_FN_DEF_EXTERN;
        }

        [[nodiscard]]
        bool is_variadic() const
        {
            return this->_flags & SRFLAG_FN_DEF_VARIADIC;
        }

        [[nodiscard]]
        bool is_mutable() const
        {
            return this->_flags & SRFLAG_FN_DEF_MUTABLE;
        }

        [[nodiscard]]
        bool is_anonymous() const
        {
            return this->_flags & SRFLAG_FN_DEF_ANONYMOUS;
        }

        llvm::Value* codegen(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        void validate() override;

        void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    class AstFunctionDeclaration
        : public IAstCallable
    {
    public:
        explicit AstFunctionDeclaration(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::shared_ptr<IAstType> return_type,
            const int flags
        ) :
            IAstCallable(
                symbol.symbol_position,
                context,
                std::move(symbol),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                flags) {}

        std::string to_string() override;

        ~AstFunctionDeclaration() override = default;

    private:
        std::optional<std::vector<llvm::Type*>> resolve_parameter_types(llvm::Module* module) const;
    };

    class AstLambdaFunctionExpression
        : public IAstCallable
    {
    public:
        explicit AstLambdaFunctionExpression(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::shared_ptr<IAstType> return_type,
            const int flags
        ) :
            IAstCallable(
                symbol.symbol_position,
                context,
                std::move(symbol),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                flags
            ) {}

        std::string to_string() override;

        ~AstLambdaFunctionExpression() override = default;
    };

    std::unique_ptr<AstFunctionDeclaration> parse_fn_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        VisibilityModifier modifier
    );

    std::unique_ptr<AstFunctionParameter> parse_standalone_fn_param(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    void parse_function_parameters(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters,
        int& function_flags
    );
} // namespace stride::ast
