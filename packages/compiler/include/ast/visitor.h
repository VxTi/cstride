#pragma once

#include "traversal.h"

#include <map>
#include <string>
#include <vector>

namespace stride::ast
{
    class AstTypeDefinition;
    class AstFunctionCall;
    class AstImport;
    class AstPackage;
    class AstFunctionDeclaration;
    class IAstExpression;
    class Ast;

    /// Visitor interface for expression nodes.
    /// Implementations receive each expression after all its child expressions
    /// have already been visited (bottom-up, post-order traversal).
    class IVisitor
    {
    public:
        std::shared_ptr<SymbolTable> current_symbol_table{};

        std::string context_name;
        ContextType current_context_type = ContextType::GLOBAL;

        std::vector<std::shared_ptr<SymbolTable>> symbol_table_stack{};

        virtual ~IVisitor() = default;

        /// Called for every expression node, after its sub-expressions have been visited.
        virtual void accept_expression(SymbolTable* symbol_table, IAstExpression* expr) {};

        virtual void accept_import_node(SymbolTable* symbol_table, AstImport* node) {}

        virtual void accept_package_node(SymbolTable* symbol_table, AstPackage* node) {}

        virtual void accept_function_declaration_node(SymbolTable* symbol_table, IAstFunction* function) {}

        virtual void accept_function_call_node(SymbolTable* symbol_table, AstFunctionCall* function_call) {}

        virtual void accept(SymbolTable* symbol_table, IAstNode* node) {}

        virtual void accept_type_definition_node(SymbolTable* symbol_table, AstTypeDefinition* type_definition) {}
    };

    /// Visitor that infers and assigns types to every expression node in the AST.
    ///
    /// Used together with AstNodeTraverser: the traverser drives bottom-up traversal,
    /// and this visitor is called for each expression after all of its child expressions
    /// have already been typed.  In addition to setting types, the visitor registers
    /// variable declarations and function declarations in the appropriate parsing context,
    /// matching the work previously performed by the scattered resolve_types() calls.
    class TypeInferenceVisitor : public IVisitor
    {
    public:
        /// Infers the type of `expr` and stores it on the node.
        /// For AstVariableDeclaration: also registers the variable in its context.
        /// For IAstFunction: also registers the function in its context.
        void accept_expression(SymbolTable* symbol_table, IAstExpression* expr) override;

        void accept_function_declaration_node(SymbolTable* symbol_table, IAstFunction* function) override;
    };

    class ValidationVisitor : public IVisitor
    {
    public:
        void accept(SymbolTable* symbol_table, IAstNode* node) override
        {
            node->validate(symbol_table);
        }
    };

    class SymbolResolver : public IVisitor
    {
        std::vector<GenericFunctionTemplate> _generic_function_instantiations;

    public:
        explicit SymbolResolver(
            std::vector<GenericFunctionTemplate> generic_function_instantiations
        ) :
            _generic_function_instantiations(std::move(generic_function_instantiations)) {}

        void accept_function_declaration_node(SymbolTable* symbol_table, IAstFunction* function) override;

        void accept_expression(SymbolTable* symbol_table, IAstExpression* expr) override;

        void accept_type_definition_node(SymbolTable* symbol_table, AstTypeDefinition* type_definition) override;

        void validate_generic_instantiations();
    };

    /// Intentionally separate from `FunctionVisitor`, as this step has to be performed after all functions have been defined.
    class TemplateInstantiator : public IVisitor
    {
        [[nodiscard]]
        bool has_instantiation(
            const std::string& function_name,
            const std::vector<std::unique_ptr<IAstType>>& generic_parameter_types
        ) const
        {
            return this->_instantiations.contains(format_generic_function_instantiation(function_name, generic_parameter_types));
        }

        void add_generic_instantiation(
            const std::string& function_name,
            const std::vector<std::unique_ptr<IAstType>>& generic_types,
            IAstNode* node
        );

        [[nodiscard]]
        static std::string format_generic_function_instantiation(
            std::string function_name,
            const std::vector<std::unique_ptr<IAstType>>& generic_parameter_types
        );

        std::map<std::string, GenericFunctionTemplate> _instantiations{};

    public:
        std::vector<GenericFunctionTemplate> get_generic_function_templates() const;

        std::vector<GenericTypeTemplate> get_generic_type_templates() const;

        void accept_function_call_node(SymbolTable* symbol_table, AstFunctionCall* function_call) override;
    };

    class ForwardReferenceInitializer : public IVisitor
    {
        llvm::Module* _module;
        llvm::IRBuilderBase* _ir_builder;

    public:
        explicit ForwardReferenceInitializer(
            llvm::Module* module,
            llvm::IRBuilderBase* ir_builder
        ) :
            _module(module),
            _ir_builder(ir_builder) {}

        void accept(SymbolTable* symbol_table, IAstNode* node) override;
    };

    class ImportVisitor : public IVisitor
    {
        std::string _current_file_name; // temporary values
        std::map<
            std::string,             /* package_name */
            std::vector<std::string> /* file_names   */
        > _package_file_mapping;
        std::map<
            std::string, /* file_name */
            std::map<
                std::string,             /* package_name */
                std::vector<std::string> /* module_names::functions/types */
            >
        > _import_registry;

    public:
        void set_current_file_name(const std::string& file_name)
        {
            this->_current_file_name = file_name;
        }

        void accept_import_node(SymbolTable* symbol_table, AstImport* node) override;

        void accept_package_node(SymbolTable* symbol_table, AstPackage* node) override;

        void cross_register_symbols(Ast* ast) const;
    };
}
