#pragma once

#include "ast_node.h"
#include "blocks.h"
#include "expression.h"

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

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

        std::unique_ptr<IAstNode> clone() override;
    };

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function declaration definitions             *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class IAstFunction
        : public IAstContainer,
          public IAstExpression
    {
        std::unique_ptr<AstBlock> _body;
        Symbol _symbol;
        std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        std::unique_ptr<IAstType> _return_type;
        std::vector<Symbol> _captured_variables;
        int _flags;

        friend class AstFunctionDeclaration;
        friend class AstFunctionParameter;

    public:
        explicit IAstFunction(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<IAstType> return_type,
            const int flags
        ) :
            IAstExpression(source, context),
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
        std::vector<std::unique_ptr<AstFunctionParameter>> get_parameters() const
        {
            std::vector<std::unique_ptr<AstFunctionParameter>> cloned_params;
            for (const auto& param : this->_parameters)
            {
                cloned_params.push_back(param->clone_as<AstFunctionParameter>());
            }

            return cloned_params;
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

        [[nodiscard]]
        const std::vector<Symbol>& get_captured_variables() const
        {
            return this->_captured_variables;
        }

        void add_captured_variable(const Symbol& symbol)
        {
            this->_captured_variables.push_back(symbol);
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        void validate() override;

        void resolve_types() override;

        void resolve_forward_references(
            ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstFunctionDeclaration
        : public IAstFunction,
          public IAstStatement
    {
    public:
        using IAstStatement::IAstStatement;

        explicit AstFunctionDeclaration(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<IAstType> return_type,
            const int flags
        ) :
            IAstFunction(
                symbol.symbol_position,
                context,
                std::move(symbol),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                flags
            ) {}

        std::string to_string() override;

        ~AstFunctionDeclaration() override = default;

    private:
        std::optional<std::vector<llvm::Type*>> resolve_parameter_types(llvm::Module* module) const;
    };

    class AstLambdaFunctionExpression
        : public IAstFunction
    {
    public:
        explicit AstLambdaFunctionExpression(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<IAstType> return_type,
            const int flags
        ) :
            IAstFunction(
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

    void parse_standalone_fn_param(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );

    void parse_function_parameters(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters,
        int& function_flags
    );
} // namespace stride::ast
