#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "symbols.h"
#include "ast/nodes/types.h"

namespace stride::ast
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

    class ISymbolDef
    {
        Symbol _symbol;

    public:
        explicit ISymbolDef(Symbol symbol) :
            _symbol(std::move(symbol)) {}

        virtual ~ISymbolDef() = default;

        [[nodiscard]]
        std::string get_internal_symbol_name() const { return this->_symbol.internal_name; }

        [[nodiscard]]
        Symbol get_symbol() const { return this->_symbol; }
    };

    class IdentifiableSymbolDef
        : public ISymbolDef
    {
        SymbolType _type;

    public:
        explicit IdentifiableSymbolDef(
            const SymbolType type,
            const Symbol& symbol
        ) : ISymbolDef(symbol),
            _type(type) {}

        [[nodiscard]]
        SymbolType get_symbol_type() const { return this->_type; }
    };

    class StructSymbolDef
        : public ISymbolDef
    {
        std::optional<Symbol> _reference_struct_sym;
        std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> _fields;

    public:
        explicit StructSymbolDef(
            Symbol struct_symbol,
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> fields // We wish to preserve order.
        ) : ISymbolDef(std::move(struct_symbol)),
            _fields(std::move(fields)) {}

        explicit StructSymbolDef(
            const Symbol& struct_name,
            const Symbol& reference_struct_name
        )
            : ISymbolDef(struct_name),
              _reference_struct_sym(reference_struct_name) {}

        [[nodiscard]]
        std::vector<std::pair<std::string, IAstType*>> get_fields() const;

        [[nodiscard]]
        std::optional<IAstType*> get_field_type(const std::string& field_name);

        static std::optional<IAstType*> get_field_type(
            const std::string& field_name,
            const std::vector<std::pair<std::string, IAstType*>>& fields
        );

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
            return get_field_type(member_name).has_value();
        }

        [[nodiscard]]
        std::optional<int> get_member_index(const std::string& member_name) const;
    };

    /// Can be either a variable or a field in a struct/class
    class FieldSymbolDef
        : public ISymbolDef
    {
        std::unique_ptr<IAstType> _type;

    public:
        explicit FieldSymbolDef(
            const Symbol& symbol,
            std::unique_ptr<IAstType> type
        ) : ISymbolDef(symbol),
            _type(std::move(type)) {}

        [[nodiscard]]
        IAstType* get_type() const { return this->_type.get(); }
    };

    class SymbolFnDefinition
        : public ISymbolDef
    {
        std::vector<std::unique_ptr<IAstType>> _parameter_types;
        std::unique_ptr<IAstType> _return_type;

    public:
        explicit SymbolFnDefinition(
            std::vector<std::unique_ptr<IAstType>> parameter_types,
            std::unique_ptr<IAstType> return_type,
            const Symbol& symbol
        ) :
            ISymbolDef(symbol),
            _parameter_types(std::move(parameter_types)),
            _return_type(std::move(return_type)) {}

        [[nodiscard]]
        std::vector<IAstType*> get_parameter_types() const
        {
            std::vector<IAstType*> out;
            out.reserve(this->_parameter_types.size());
            for (const auto& p : this->_parameter_types)
                out.push_back(p.get());
            return out;
        }

        [[nodiscard]]
        IAstType* get_return_type() const { return this->_return_type.get(); }

        ~SymbolFnDefinition() override = default;
    };

    class ParsingContext
    {
        /**
         * Name of the context. This can be used for function name mangling,
         * e.g., in the context of modules.
         */
        std::string _context_name;
        ScopeType _current_scope;
        std::shared_ptr<ParsingContext> _parent_registry;

        std::vector<std::unique_ptr<ISymbolDef>> _symbols;

    public:
        explicit ParsingContext(
            std::string context_name,
            const ScopeType type,
            std::shared_ptr<ParsingContext> parent
        ) : _context_name(std::move(context_name)),
            _current_scope(type),
            _parent_registry(std::move(parent)) {}

        /// Non-specific scope context definitions, e.g., for/while-loop blocks
        explicit ParsingContext(
            std::shared_ptr<ParsingContext> parent,
            const ScopeType type
        ) : ParsingContext(parent->_context_name, type, std::move(parent)) // Context gets the same name as the parent
        {}

        /// Root node initialization
        explicit ParsingContext()
            : ParsingContext("", ScopeType::GLOBAL, nullptr) {}

        ParsingContext& operator=(const ParsingContext&) = delete;

        [[nodiscard]]
        ScopeType get_current_scope_type() const { return this->_current_scope; }

        bool is_global_scope() const { return this->_current_scope == ScopeType::GLOBAL; }

        [[nodiscard]]
        const FieldSymbolDef* get_variable_def(const std::string& variable_name) const;

        [[nodiscard]]
        const SymbolFnDefinition* get_function_def(const std::string& function_name) const;

        [[nodiscard]]
        const IdentifiableSymbolDef* get_symbol_def(const std::string& symbol_name) const;

        [[nodiscard]]
        std::optional<StructSymbolDef*> get_struct_def(const std::string& name) const;

        [[nodiscard]]
        std::optional<std::vector<std::pair<std::string, IAstType*>>> get_struct_fields(const std::string& name) const;

        [[nodiscard]]
        ParsingContext* get_parent_registry() const { return this->_parent_registry.get(); }

        [[nodiscard]]
        const FieldSymbolDef* lookup_variable(const std::string& name) const;

        /// Will attempt to define the function in the global context.
        void define_function(
            const Symbol& symbol,
            std::vector<std::unique_ptr<IAstType>> parameter_types,
            std::unique_ptr<IAstType> return_type
        ) const;

        void define_struct(
            const std::string& struct_name,
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> fields
        ) const;

        void define_struct(
            const Symbol& struct_name,
            const Symbol& reference_struct_name
        ) const;

        void define_variable(
            std::string variable_name,
            const std::string& internal_name,
            std::unique_ptr<IAstType> type
        );

        [[nodiscard]]
        ISymbolDef* fuzzy_find(const std::string& symbol_name) const;

        void define_symbol(const Symbol& symbol_name, SymbolType type);

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
        bool is_function_defined_globally(const std::string& internal_function_name) const;

        [[nodiscard]]
        std::string get_name() const { return this->_context_name; }

    private:
        [[nodiscard]]
        const ParsingContext& traverse_to_root() const;
    };

    std::string scope_type_to_str(const ScopeType& scope_type);
}
