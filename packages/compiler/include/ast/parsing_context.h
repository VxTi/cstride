#pragma once
#include "ast/nodes/types.h"
#include "symbols.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace stride::ast
{
    namespace definition
    {
        enum class ScopeType
        {
            GLOBAL,
            MODULE,
            FUNCTION,
            CLASS,
            BLOCK
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
            explicit IdentifiableSymbolDef(const SymbolType type,
                                           const Symbol& symbol) :
                IDefinition(symbol),
                _type(type) {}

            [[nodiscard]]
            SymbolType get_symbol_type() const
            {
                return this->_type;
            }
        };

        class StructDef : public IDefinition
        {
            std::optional<Symbol> _reference_struct_sym;
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>>
            _fields;

        public:
            explicit StructDef(
                Symbol struct_symbol,
                std::vector<std::pair<std::string, std::unique_ptr<IAstType>>>
                fields // We wish to preserve order.
            ) :
                IDefinition(std::move(struct_symbol)),
                _fields(std::move(fields)) {}

            explicit StructDef(const Symbol& struct_name,
                               const Symbol& reference_struct_name) :
                IDefinition(struct_name),
                _reference_struct_sym(reference_struct_name) {}

            [[nodiscard]]
            std::vector<std::pair<std::string, IAstType*>> get_fields() const;

            [[nodiscard]]
            std::optional<IAstType*> get_struct_member_field_type(
                const std::string& field_name);

            static std::optional<IAstType*> get_struct_member_field_type(
                const std::string& field_name,
                const std::vector<std::pair<std::string, IAstType*>>& fields);

            [[nodiscard]]
            bool is_reference_struct() const;

            [[nodiscard]]
            std::optional<Symbol> get_reference_struct() const
            {
                return this->_reference_struct_sym;
            }

            [[nodiscard]]
            bool has_member(const std::string& member_name)
            {
                return get_struct_member_field_type(member_name).has_value();
            }

            [[nodiscard]]
            std::optional<int> get_struct_field_member_index(
                const std::string& member_name) const;
        };

        class FieldDef : public IDefinition
        {
            std::unique_ptr<IAstType> _type;

            /// Can be either a variable or a field in a struct/class
        public:
            explicit FieldDef(const Symbol& symbol,
                              std::unique_ptr<IAstType> type) :
                IDefinition(symbol),
                _type(std::move(type)) {}

            [[nodiscard]]
            IAstType* get_type() const
            {
                return this->_type.get();
            }
        };

        class CallableDef : public IDefinition
        {
            std::unique_ptr<AstFunctionType> _function_type;

        public:
            explicit CallableDef(
                std::unique_ptr<AstFunctionType> function_type,
                const Symbol& symbol) :
                IDefinition(symbol),
                _function_type(std::move(function_type)) {}

            [[nodiscard]]
            AstFunctionType* get_type() const
            {
                return this->_function_type.get();
            }

            ~CallableDef() override = default;
        };
    } // namespace definition

    class ParsingContext
    {
        /**
         * Name of the context. This can be used for function name mangling,
         * e.g., in the context of modules.
         */
        std::string _context_name;
        definition::ScopeType _current_scope;
        std::shared_ptr<ParsingContext> _parent_registry;

        std::vector<std::unique_ptr<definition::IDefinition>> _symbols;

    public:
        explicit ParsingContext(
            std::string context_name,
            const definition::ScopeType type,
            std::shared_ptr<ParsingContext> parent) :
            _context_name(std::move(context_name)),
            _current_scope(type),
            _parent_registry(std::move(parent)) {}

        /// Non-specific scope context definitions, e.g., for/while-loop blocks
        explicit ParsingContext(
            std::shared_ptr<ParsingContext> parent,
            const definition::ScopeType type) :
            ParsingContext(
                parent->_context_name,
                type,
                std::move(parent)) // Context gets the same name as the parent
        {}

        /// Root node initialization
        explicit ParsingContext() :
            ParsingContext("", definition::ScopeType::GLOBAL, nullptr) {}

        ParsingContext& operator=(const ParsingContext&) = delete;

        [[nodiscard]]
        definition::ScopeType get_current_scope_type() const
        {
            return this->_current_scope;
        }

        [[nodiscard]]
        bool is_global_scope() const
        {
            // We deem module scope as global as well
            return this->_current_scope == definition::ScopeType::GLOBAL ||
                this->_current_scope == definition::ScopeType::MODULE;
        }

        [[nodiscard]]
        const definition::FieldDef* get_variable_def(
            const std::string& variable_name,
            bool use_raw_name = false) const;

        [[nodiscard]]
        const definition::CallableDef* get_function_def(
            const std::string& function_name) const;

        [[nodiscard]]
        std::optional<definition::StructDef*> get_struct_def(
            const std::string& name) const;

        [[nodiscard]]
        const definition::IdentifiableSymbolDef* get_symbol_def(
            const std::string& symbol_name) const;


        [[nodiscard]]
        std::optional<std::vector<std::pair<std::string, IAstType*>>>
        get_struct_fields(
            const std::string& name) const;

        [[nodiscard]]
        ParsingContext* get_parent_registry() const
        {
            return this->_parent_registry.get();
        }

        [[nodiscard]]
        const definition::FieldDef* lookup_variable(
            const std::string& name,
            bool use_raw_name = false) const;

        definition::IDefinition* lookup_symbol(
            const std::string& symbol_name) const;

        /// Will attempt to define the function in the global context.
        void define_function(Symbol function_name,
                             std::unique_ptr<AstFunctionType> function_type)
        const;

        void define_struct(
            const Symbol& struct_symbol,
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>>
            fields) const;

        void define_struct(const Symbol& struct_name,
                           const Symbol& reference_struct_name) const;

        void define_variable(Symbol variable_sym,
                             std::unique_ptr<IAstType> type) const;

        void define_variable_globally(Symbol variable_symbol,
                                      std::unique_ptr<IAstType> type) const;

        [[nodiscard]]
        definition::IDefinition* fuzzy_find(
            const std::string& symbol_name) const;

        void define_symbol(const Symbol& symbol_name,
                           definition::SymbolType type);


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
            const std::string& internal_function_name) const;

        [[nodiscard]]
        std::string get_name() const
        {
            return this->_context_name;
        }

    private:
        [[nodiscard]]
        const ParsingContext& traverse_to_root() const;
    };

    std::string scope_type_to_str(const definition::ScopeType& scope_type);
} // namespace stride::ast
