#pragma once

#include "modifiers.h"
#include "symbols.h"
#include "ast/nodes/types.h"
#include "definitions/definitions.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <llvm/IR/Function.h>

namespace llvm
{
    class BasicBlock;
}

namespace stride::ast
{
    enum class ContextType
    {
        GLOBAL,
        MODULE,
        FUNCTION,
        CLASS,
        CONTROL_FLOW
    };

    // ----------------------------------------------------------------------------------- //
    //                                                                                     //
    //                          Template instantiation types                               //
    //                                                                                     //
    //     Generics types / functions are all represented as a template instantiation.     //
    //     Their runtime types are known after template resolution, but they are still     //
    //     represented as a template instantiation during parsing and semantic analysis.   //
    //                                                                                     //
    // ----------------------------------------------------------------------------------- //

    enum class TemplateInstantiationType
    {
        GENERIC_FUNCTION,
        GENERIC_TYPE
    };

    struct TemplateInstantiation
    {
        TemplateInstantiationType type;
        std::string symbol_name;
        std::vector<std::unique_ptr<IAstType>> instantiated_types;
    };

    // ----------------------------------------------------------------------------------- //
    //                                                                                     //
    //                       Context for symbol definitions and lookups                    //
    //                                                                                     //
    //  The Context class represents a scope in the program, which can be a global scope,  //
    //  a module, a function, a class, or a control flow block. Each context maintains a   //
    //  registry of symbols defined within that scope, as well as a reference to its       //
    //  parent context, allowing for nested scopes and symbol resolution.                  //
    //                                                                                     //
    // ----------------------------------------------------------------------------------- //

    class SymbolTable
    {
        /**
         * Name of the context. This can be used for function name mangling,
         * e.g., in the context of modules.
         */
        std::string _context_name;
        ContextType _context_type;
        std::shared_ptr<SymbolTable> _parent_registry;

        std::vector<std::unique_ptr<definition::IDefinition>> _symbols;

        // Stack of loop blocks for break and continue: pair<continue_block, break_block>
        // This isn't used during parsing, hence it not needing to be moved when creating a new ParsingContext.
        static inline std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> control_flow_loop_blocks;

        std::vector<TemplateInstantiation> _template_instantiations;

    public:
        explicit SymbolTable(
            std::string context_name,
            const ContextType type,
            std::shared_ptr<SymbolTable> parent
        ) :
            _context_name(std::move(context_name)),
            _context_type(type),
            _parent_registry(std::move(parent)) {}

        /// Non-specific scope context definitions, e.g., for/while-loop blocks
        explicit SymbolTable(
            std::shared_ptr<SymbolTable> parent,
            const ContextType type
        ) :
            // Context gets the same name as the parent
            SymbolTable(parent->_context_name, type, std::move(parent)) {}

        /// Root node initialization
        explicit SymbolTable() :
            SymbolTable("", ContextType::GLOBAL, nullptr) {}

        SymbolTable& operator=(const SymbolTable&) = delete;

        [[nodiscard]]
        ContextType get_context_type() const
        {
            return this->_context_type;
        }

        [[nodiscard]]
        bool is_global_scope() const
        {
            // We deem module scope as global as well
            return this->_context_type == ContextType::GLOBAL
                || this->_context_type == ContextType::MODULE;
        }

        static void push_control_flow_block(
            llvm::BasicBlock* continue_block,
            llvm::BasicBlock* break_block)
        {
            control_flow_loop_blocks.emplace_back(continue_block, break_block);
        }

        static void pop_control_flow_block()
        {
            control_flow_loop_blocks.pop_back();
        }

        static std::pair<llvm::BasicBlock*, llvm::BasicBlock*> get_current_control_flow_block()
        {
            return control_flow_loop_blocks.back();
        }

        static std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> get_control_flow_blocks()
        {
            return control_flow_loop_blocks;
        }

        [[nodiscard]]
        definition::FieldDefinition* get_variable_def(
            const std::string& variable_name,
            bool use_raw_name = false
        ) const;

        /// Primarily used for function invocations, where the parameter types are known,
        /// but we don't yet know what the return type is.
        [[nodiscard]]
        std::optional<definition::FunctionDefinition*> get_function_definition(
            const std::string& function_name,
            const std::vector<std::unique_ptr<IAstType>>& parameter_types,
            size_t instantiated_generic_count = 0
        ) const;

        [[nodiscard]]
        std::optional<definition::FunctionDefinition*> get_generic_function_definition(
            const std::string& function_name,
            size_t instantiated_generic_count = 0
        ) const;

        std::optional<definition::FunctionDefinition*> get_function_definition(
            const std::string& function_name,
            IAstType* function_type
        ) const;

        [[nodiscard]]
        std::optional<definition::TypeDefinition*> get_type_definition(
            const std::string& name
        ) const;

        [[nodiscard]]
        std::optional<AstObjectType*> get_object_type(const std::string& name) const;

        [[nodiscard]]
        std::optional<std::unique_ptr<definition::IDefinition>> get_definition_by_internal_name(
            const std::string& internal_name) const;

        [[nodiscard]]
        SymbolTable* get_parent_context() const
        {
            if (this->_parent_registry == nullptr)
                return nullptr;

            return this->_parent_registry.get();
        }

        [[nodiscard]]
        definition::FieldDefinition* lookup_variable(
            const std::string& name,
            bool use_raw_name = false
        ) const;

        [[nodiscard]]
        definition::IDefinition* lookup_symbol(const std::string& symbol_name) const;

        /// Will attempt to define the function in the global context.
        void define_function(
            Symbol function_name,
            std::unique_ptr<AstFunctionType> function_type,
            VisibilityModifier visibility,
            int flags = SRFLAG_NONE
        ) const;

        void define_type(
            const Symbol& type_name,
            std::unique_ptr<IAstType> type,
            GenericParameterList generics,
            VisibilityModifier visibility
        ) const;

        void define_variable(
            Symbol variable_sym,
            std::unique_ptr<IAstType> type,
            VisibilityModifier visibility,
            bool overwrite = false
        );

        void define_variable_globally(
            Symbol variable_symbol,
            std::unique_ptr<IAstType> type,
            VisibilityModifier visibility,
            bool overwrite = false
        ) const;

        [[nodiscard]]
        definition::IDefinition* fuzzy_find(const std::string& symbol_name) const;

        [[nodiscard]]
        bool is_struct_type_defined(const std::string& struct_name) const;

        [[nodiscard]]
        bool is_type_defined(const std::string& type_name) const;

        void define(std::unique_ptr<definition::IDefinition> definition);

        /// Checks whether the provided variable name is defined in the current context.
        [[nodiscard]]
        bool is_field_defined_in_scope(const std::string& variable_name) const;

        /// Checks whether the provided variable name is defined in the global context.
        [[nodiscard]]
        bool is_field_defined_globally(const std::string& field_name) const;

        /// Checks whether the provided internal function name is defined in the global context.
        /// Do note that the internal name is not the name that you would use in
        /// source code, but rather the mangled name used for code generation.
        [[nodiscard]]
        bool is_function_defined_globally(
            const std::string& function_name,
            const AstFunctionType* function_type
        ) const;

        [[nodiscard]]
        std::string get_scope_name() const
        {
            return this->_context_name;
        }

        [[nodiscard]]
        const SymbolTable& traverse_to_root() const;
    };

    std::string scope_type_to_str(const ContextType& scope_type);
} // namespace stride::ast
