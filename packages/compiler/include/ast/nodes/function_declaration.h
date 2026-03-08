#pragma once

#include "ast_node.h"
#include "blocks.h"
#include "expression.h"
#include "ast/modifiers.h"

#include <utility>

namespace llvm
{
    class Function;
}

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
        std::unique_ptr<IAstType> _annotated_return_type;
        std::vector<Symbol> _captured_variables;
        VisibilityModifier _visibility;
        GenericParameterList _generic_parameters;
        int _flags;

        /// Cached LLVM function pointer for anonymous functions.
        /// Named functions are always looked up by their scoped name in the module,
        /// but anonymous functions are created with an empty name (LLVM auto-assigns
        /// a numeric ID), so we must track them by pointer instead.
        llvm::Function* _llvm_function = nullptr;

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
            const VisibilityModifier visibility,
            const int flags,
            const GenericParameterList& generic_parameters
        ) :
            IAstExpression(source, context),
            _body(std::move(body)),
            _symbol(std::move(symbol)),
            _parameters(std::move(parameters)),
            _annotated_return_type(std::move(return_type)),
            _visibility(visibility),
            _generic_parameters(generic_parameters),
            _flags(flags) {}

        [[nodiscard]]
        const std::string& get_function_name() const
        {
            return this->_symbol.name;
        }

        [[nodiscard]]
        const std::string& get_scoped_function_name() const
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
            cloned_params.reserve(this->_parameters.size());

            for (const auto& param : this->_parameters)
            {
                cloned_params.push_back(param->clone_as<AstFunctionParameter>());
            }

            return cloned_params;
        }

        /// Returns a non-owning const reference to the parameter list, avoiding the
        /// clone overhead of get_parameters() when only read access is needed.
        [[nodiscard]]
        const std::vector<std::unique_ptr<AstFunctionParameter>>& get_parameters_ref() const
        {
            return this->_parameters;
        }

        [[nodiscard]]
        Symbol get_symbol() const
        {
            return this->_symbol;
        }

        [[nodiscard]]
        IAstType* get_return_type() const
        {
            return this->_annotated_return_type.get();
        }

        [[nodiscard]]
        bool is_extern() const
        {
            return this->_flags & SRFLAG_FN_TYPE_EXTERN;
        }

        [[nodiscard]]
        bool is_variadic() const
        {
            return this->_flags & SRFLAG_FN_TYPE_VARIADIC;
        }

        [[nodiscard]]
        bool is_anonymous() const
        {
            return this->_flags & SRFLAG_FN_TYPE_ANONYMOUS;
        }

        [[nodiscard]]
        bool is_private() const
        {
            return this->_visibility == VisibilityModifier::PRIVATE;
        }

        [[nodiscard]]
        VisibilityModifier get_visibility() const
        {
            return this->_visibility;
        }

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }

        [[nodiscard]]
        const std::vector<std::string>& get_generic_parameters() const
        {
            return this->_generic_parameters;
        }

        [[nodiscard]]
        bool is_generic_function() const
        {
            return !this->_generic_parameters.empty();
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

        void resolve_forward_references(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::unique_ptr<IAstNode> clone() override;

    private:
        llvm::FunctionType* get_llvm_function_type(llvm::Module* module) const;

        llvm::FunctionType* get_llvm_function_type(
            llvm::Module* module,
            std::vector<llvm::Type*> captured_variables
        ) const;

        std::optional<std::vector<llvm::Type*>> get_llvm_function_parameter_types(llvm::Module* module) const;
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
            const VisibilityModifier visibility,
            const int flags,
            const GenericParameterList& generic_parameters
        ) :
            IAstFunction(
                symbol.symbol_position,
                context,
                std::move(symbol),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                visibility,
                flags,
                generic_parameters
            ) {}

        std::string to_string() override;

        ~AstFunctionDeclaration() override = default;
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
            const VisibilityModifier visibility,
            const int flags
        ) :
            IAstFunction(
                symbol.symbol_position,
                context,
                std::move(symbol),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                visibility,
                flags,
                {}
            ) {}

        std::string to_string() override;

        ~AstLambdaFunctionExpression() override = default;

        std::string get_mangled_name() const { return ""; }; // TODO: Implement
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
