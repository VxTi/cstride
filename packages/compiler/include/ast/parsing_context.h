#pragma once

#include "symbols.h"
#include "ast/nodes/types.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace llvm
{
    class BasicBlock;
}

namespace stride::ast
{
    namespace definition
    {
        using StructFieldPair = std::pair<std::string, std::unique_ptr<IAstType>>;

        enum class ContextType
        {
            GLOBAL,
            MODULE,
            FUNCTION,
            CLASS,
            CONTROL_FLOW
        };

        enum class SymbolType
        {
            CLASS,
            VARIABLE,
            ENUM,
            ENUM_MEMBER,
            STRUCT,
            STRUCT_MEMBER
        };

        class IDefinition
        {
            Symbol _symbol;

        public:
            explicit IDefinition(Symbol symbol) :
                _symbol(std::move(symbol)) {}

            virtual ~IDefinition() = default;

            [[nodiscard]]
            std::string get_internal_symbol_name() const
            {
                return this->_symbol.internal_name;
            }

            [[nodiscard]]
            Symbol get_symbol() const
            {
                return this->_symbol;
            }
        };

        class IdentifiableSymbolDef : public IDefinition
        {
            SymbolType _type;

        public:
            explicit IdentifiableSymbolDef(
                const SymbolType type,
                const Symbol& symbol
            ) :
                IDefinition(symbol),
                _type(type) {}

            [[nodiscard]]
            SymbolType get_symbol_type() const
            {
                return this->_type;
            }
        };

        class TypeDefinition
            : public IDefinition
        {
            std::unique_ptr<IAstType> _type;

        public:
            explicit TypeDefinition(
                Symbol type_name_symbol,
                std::unique_ptr<IAstType> type
            ) :
                IDefinition(std::move(type_name_symbol)),
                _type(std::move(type)) {}

            [[nodiscard]]
            IAstType* get_type() const
            {
                return this->_type.get();
            }
        };

        class FieldDefinition : public IDefinition
        {
            std::unique_ptr<IAstType> _type;

            /// Can be either a variable or a field in a struct/class
        public:
            explicit FieldDefinition(const Symbol& symbol,
                                     std::unique_ptr<IAstType> type) :
                IDefinition(symbol),
                _type(std::move(type)) {}

            [[nodiscard]]
            IAstType* get_type() const
            {
                return this->_type.get();
            }

            [[nodiscard]]
            std::string get_field_name() const
            {
                return this->get_symbol().name;
            }
        };

        class FunctionDefinition
            : public IDefinition
        {
            std::unique_ptr<AstFunctionType> _function_type;
            int _flags;

        public:
            explicit FunctionDefinition(
                std::unique_ptr<AstFunctionType> function_type,
                const Symbol& symbol,
                const int flags
            ) :
                IDefinition(symbol),
                _function_type(std::move(function_type)),
                _flags(flags) {}

            [[nodiscard]]
            AstFunctionType* get_type() const
            {
                return this->_function_type.get();
            }

            [[nodiscard]]
            std::string get_function_name() const
            {
                return this->get_symbol().name;
            }

            [[nodiscard]]
            int get_flags() const
            {
                return this->_flags;
            }

            ~FunctionDefinition() override = default;

            bool matches_signature(const std::string& name, const AstFunctionType* signature) const;

            [[nodiscard]]
            bool matches_signature(
                const std::string& function_name,
                const std::vector<std::unique_ptr<IAstType>>& other_parameter_types
            ) const;
        };
    } // namespace definition

    class ParsingContext
    {
        /**
         * Name of the context. This can be used for function name mangling,
         * e.g., in the context of modules.
         */
        std::string _context_name;
        definition::ContextType _context_type;
        std::shared_ptr<ParsingContext> _parent_registry;

        std::vector<std::unique_ptr<definition::IDefinition>> _symbols;

        // Stack of loop blocks for break and continue: pair<continue_block, break_block>
        // This isn't used during parsing, hence it not needing to be moved when creating a new ParsingContext.
        static inline
        std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> control_flow_loop_blocks;

    public:
        explicit ParsingContext(
            std::string context_name,
            const definition::ContextType type,
            std::shared_ptr<ParsingContext> parent) :
            _context_name(std::move(context_name)),
            _context_type(type),
            _parent_registry(std::move(parent)) {}

        /// Non-specific scope context definitions, e.g., for/while-loop blocks
        explicit ParsingContext(
            std::shared_ptr<ParsingContext> parent,
            const definition::ContextType type
        ) :
            // Context gets the same name as the parent
            ParsingContext(parent->_context_name, type, std::move(parent)) {}

        /// Root node initialization
        explicit ParsingContext() :
            ParsingContext("", definition::ContextType::GLOBAL, nullptr) {}

        ParsingContext& operator=(const ParsingContext&) = delete;

        [[nodiscard]]
        definition::ContextType get_context_type() const
        {
            return this->_context_type;
        }

        [[nodiscard]]
        bool is_global_scope() const
        {
            // We deem module scope as global as well
            return this->_context_type == definition::ContextType::GLOBAL
                || this->_context_type == definition::ContextType::MODULE;
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

        static std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>
        get_control_flow_blocks()
        {
            return control_flow_loop_blocks;
        }

        [[nodiscard]]
        const definition::FieldDefinition* get_variable_def(
            const std::string& variable_name,
            bool use_raw_name = false
        ) const;

        /// Primarily used for function invocations, where the parameter types are known,
        /// but we don't yet know what the return type is.
        [[nodiscard]]
        std::optional<definition::FunctionDefinition*> get_function_definition(
            const std::string& function_name,
            const std::vector<std::unique_ptr<IAstType>>& parameter_types
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
        std::optional<AstStructType*> get_struct_type(const std::string& name) const;

        [[nodiscard]]
        const definition::IdentifiableSymbolDef* get_symbol_def(
            const std::string& symbol_name
        ) const;

        [[nodiscard]]
        std::shared_ptr<ParsingContext> get_parent_context() const
        {
            return this->_parent_registry;
        }

        [[nodiscard]]
        const definition::FieldDefinition* lookup_variable(
            const std::string& name,
            bool use_raw_name = false
        ) const;

        [[nodiscard]]
        definition::IDefinition* lookup_symbol(const std::string& symbol_name) const;

        /// Will attempt to define the function in the global context.
        void define_function(
            Symbol function_name,
            std::unique_ptr<AstFunctionType> function_type,
            int flags
        ) const;

        void define_type(
            const Symbol& type_name,
            std::unique_ptr<IAstType> type
        ) const;

        void define_variable(
            Symbol variable_sym,
            std::unique_ptr<IAstType> type
        );

        void define_variable_globally(
            Symbol variable_symbol,
            std::unique_ptr<IAstType> type
        ) const;

        [[nodiscard]]
        definition::IDefinition* fuzzy_find(const std::string& symbol_name) const;

        [[nodiscard]]
        bool is_struct_type_defined(const std::string& struct_name) const;

        [[nodiscard]]
        bool is_type_defined(const std::string& type_name) const;

        void define_symbol(const Symbol& symbol_name, definition::SymbolType type);

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
        std::string get_name() const
        {
            return this->_context_name;
        }

    private:
        [[nodiscard]]
        const ParsingContext& traverse_to_root() const;
    };

    std::string scope_type_to_str(const definition::ContextType& scope_type);
} // namespace stride::ast
