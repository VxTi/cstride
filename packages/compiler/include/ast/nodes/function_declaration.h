#pragma once

#include "ast_node.h"
#include "blocks.h"
#include "expression.h"
#include "ast/modifiers.h"
#include "ast/symbol_table.h"

#include <utility>

namespace llvm
{
    class Function;
}

namespace stride::ast
{
    class AstReturnStatement;
#define MAX_FUNCTION_PARAMETERS (32)

    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
     *                                                             *
     *                Function parameter definitions               *
     *                                                             *
     * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
    class AstFunctionParameter
        : public IAstNode
    {
        Symbol _param_name_symbol;
        std::unique_ptr<IAstType> _type;

    public:
        explicit AstFunctionParameter(
            const SourcePosition& source,
            const std::string& param_name,
            std::unique_ptr<IAstType> param_type
        ) :
            IAstNode(source),
            _param_name_symbol(Symbol(source, param_name)),
            _type(std::move(param_type)) {}

        std::string to_string() override;

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_param_name_symbol.name;
        }

        [[nodiscard]]
        const Symbol& get_symbol() const
        {
            return this->_param_name_symbol;
        }

        [[nodiscard]]
        IAstType* get_type() const
        {
            return this->_type.get();
        }

        ~AstFunctionParameter() override = default;

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override
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

    struct FunctionImplementation
    {
        std::string overload_function_name;
        llvm::Function* llvm_function;
        AstBlock* body = nullptr; // Non-null for generic overloads (resolved body)
    };

    class IAstFunction
        : public IAstContainer,
          public IAstExpression
    {
        std::unique_ptr<AstBlock> _body;
        std::string _function_name;
        std::vector<std::unique_ptr<AstFunctionParameter>> _parameters;
        std::unique_ptr<IAstType> _annotated_return_type;
        std::vector<Symbol> _captured_variables;
        VisibilityModifier _visibility;
        GenericParameterList _generic_parameters;
        definition::FunctionDefinition* _function_definition = nullptr;
        int _flags;

        friend class AstFunctionDeclaration;
        friend class AstFunctionParameter;

        std::vector<std::shared_ptr<IAstFunction>> _generic_instantiations;

    public:
        explicit IAstFunction(
            const SourcePosition& source,
            std::string function_name,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<IAstType> return_type,
            const VisibilityModifier visibility,
            const int flags,
            GenericParameterList generic_parameters
        ) :
            IAstExpression(source),
            _body(std::move(body)),
            _function_name(std::move(function_name)),
            _parameters(std::move(parameters)),
            _annotated_return_type(std::move(return_type)),
            _visibility(visibility),
            _generic_parameters(std::move(generic_parameters)),
            _flags(flags) {}

        [[nodiscard]]
        const std::string& get_function_name() const
        {
            return this->_function_name;
        }

        [[nodiscard]]
        const std::vector<std::shared_ptr<IAstFunction>>& get_generic_instantiations() const
        {
            return this->_generic_instantiations;
        }

        [[nodiscard]]
        std::vector<std::unique_ptr<IAstType>> get_parameter_types() const;

        /// Returns a list of overloads for this function. For example, whenever the
        /// function is defined with generic parameters, there will be several overloads generated
        /// for each generic instantiation. This function returns the internalized name of each overload.
        [[nodiscard]]
        std::vector<FunctionImplementation> get_function_implementation_data(SymbolTable* symbol_table);

        [[nodiscard]]
        AstBlock* get_body() override
        {
            return this->_body.get();
        }

        [[nodiscard]]
        std::vector<std::unique_ptr<AstFunctionParameter>> get_parameters() const;

        /// Returns a non-owning const reference to the parameter list, avoiding the
        /// clone overhead of get_parameters() when only read access is needed.
        [[nodiscard]]
        const std::vector<std::unique_ptr<AstFunctionParameter>>& get_parameters_ref() const
        {
            return this->_parameters;
        }

        [[nodiscard]]
        IAstType* get_return_type() const
        {
            return this->_annotated_return_type.get();
        }

        [[nodiscard]]
        bool is_extern() const
        {
            return (this->_flags & SRFLAG_FN_TYPE_EXTERN) != 0;
        }

        [[nodiscard]]
        bool is_variadic() const
        {
            return (this->_flags & SRFLAG_FN_TYPE_VARIADIC) != 0;
        }

        [[nodiscard]]
        bool is_anonymous() const
        {
            return (this->_flags & SRFLAG_FN_TYPE_ANONYMOUS) != 0;
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
        const GenericParameterList& get_generic_parameters() const
        {
            return this->_generic_parameters;
        }

        [[nodiscard]]
        bool is_generic() const
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

        definition::FunctionDefinition* get_function_definition(SymbolTable* symbol_table);

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        void validate(SymbolTable* symbol_table) override;

        void resolve_forward_references(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override;

        std::shared_ptr<IAstFunction> instantiate_generic_function_template(
            const SymbolTable* symbol_table,
            const GenericTypeList& instantiated_types
        );

    private:
        llvm::FunctionType* get_llvm_function_type(
            llvm::Module* module,
            std::vector<llvm::Type*> captured_variables,
            const GenericTypeList& generic_instantiation_types = {}
        ) const;

        static void validate_candidate(SymbolTable* symbol_table, IAstFunction* candidate);

        static void collect_free_variables(
            IAstNode* node,
            SymbolTable* lambda_symbol_table,
            SymbolTable* outer_symbol_table,
            std::vector<Symbol>& captures
        );

        static std::vector<AstReturnStatement*> collect_return_statements(const AstBlock* body);
    };

    class AstFunctionDeclaration
        : public IAstFunction,
          public IAstStatement
    {
    public:
        explicit AstFunctionDeclaration(
            const SourcePosition& position,
            std::string function_name,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<IAstType> return_type,
            const VisibilityModifier visibility,
            const int flags,
            const GenericParameterList& generic_parameters = EMPTY_GENERIC_PARAMETER_LIST
        ) :
            IAstFunction(
                position,
                std::move(function_name),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                visibility,
                flags,
                generic_parameters
            ) {}

        ~AstFunctionDeclaration() override = default;
    };

    class AstLambdaFunctionExpression
        : public IAstFunction
    {
    public:
        explicit AstLambdaFunctionExpression(
            const SourcePosition& position,
            std::string function_name,
            std::vector<std::unique_ptr<AstFunctionParameter>> parameters,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<IAstType> return_type,
            const VisibilityModifier visibility,
            const int flags
        ) :
            IAstFunction(
                position,
                std::move(function_name),
                std::move(parameters),
                std::move(body),
                std::move(return_type),
                visibility,
                flags,
                {}
            ) {}

        ~AstLambdaFunctionExpression() override = default;
    };

    std::unique_ptr<AstFunctionDeclaration> parse_fn_declaration(
        TokenSet& set,
        VisibilityModifier modifier
    );

    void parse_standalone_fn_param(
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
    );

    void parse_function_parameters(
        TokenSet& set,
        std::vector<std::unique_ptr<AstFunctionParameter>>& parameters,
        int& function_flags
    );
} // namespace stride::ast
