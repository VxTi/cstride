#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

    std::string scope_type_to_str(const ScopeType& scope_type);

    class ISymbolDef
    {
        std::string _internal_name;

    public:
        explicit ISymbolDef(std::string symbol_name) :
            _internal_name(std::move(symbol_name)) {}

        virtual ~ISymbolDef() = default;

        [[nodiscard]]
        std::string get_internal_symbol_name() const { return this->_internal_name; }
    };

    class IdentifiableSymbolDef
        : public ISymbolDef
    {
        SymbolType _type;

    public:
        explicit IdentifiableSymbolDef(
            const SymbolType type,
            const std::string& symbol_name
        ) : ISymbolDef(symbol_name),
            _type(type) {}

        [[nodiscard]]
        SymbolType get_symbol_type() const { return this->_type; }
    };

    class StructSymbolDef
        : public ISymbolDef
    {
        std::optional<std::string> _reference_struct_name;
        std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> _fields;

    public:
        explicit StructSymbolDef(
            std::string struct_name,
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> fields // We wish to preserve order.
        ) : ISymbolDef(std::move(struct_name)),
            _fields(std::move(fields)) {}

        explicit StructSymbolDef(
            const std::string& struct_name,
            const std::string& reference_struct_name
        )
            : ISymbolDef(struct_name),
              _reference_struct_name(reference_struct_name) {}

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
        std::optional<std::string> get_reference_struct_name() const
        {
            return this->_reference_struct_name;
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
        std::string _variable_name;

    public:
        explicit FieldSymbolDef(
            std::string field_name,
            const std::string& internal_name,
            std::unique_ptr<IAstType> type
        ) : ISymbolDef(internal_name),
            _type(std::move(type)),
            _variable_name(std::move(field_name)) {}

        [[nodiscard]]
        IAstType* get_type() const { return this->_type.get(); }

        [[nodiscard]]
        const std::string& get_variable_name() const { return this->_variable_name; }
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
            const std::string& internal_name
        ) :
            ISymbolDef(internal_name),
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

    class SymbolRegistry
    {
        ScopeType _current_scope;
        std::shared_ptr<SymbolRegistry> _parent_registry;

        std::vector<std::unique_ptr<ISymbolDef>> _symbols;

    public:
        explicit SymbolRegistry(
            std::shared_ptr<SymbolRegistry> parent,
            const ScopeType type
        ) : _current_scope(type),
            _parent_registry(std::move(parent)) {}

        explicit SymbolRegistry(const ScopeType type)
            : SymbolRegistry(nullptr, type) {}

        [[nodiscard]]
        ScopeType get_current_scope() const { return this->_current_scope; }

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
        const FieldSymbolDef* field_lookup(const std::string& name) const;

        /// Will attempt to define the function in the global registry.
        void define_function(
            std::string internal_function_name,
            std::vector<std::unique_ptr<IAstType>> parameter_types,
            std::unique_ptr<IAstType> return_type
        ) const;

        void define_struct(
            std::string struct_name,
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> fields
        ) const;

        void define_struct(
            const std::string& struct_name,
            const std::string& reference_struct_name
        ) const;

        void define_field(
            std::string field_name,
            std::string internal_name,
            std::unique_ptr<IAstType> type
        );

        [[nodiscard]]
        ISymbolDef* fuzzy_find(const std::string& symbol_name) const;

        void define_symbol(const std::string& symbol_name, SymbolType type);

        /// Checks whether the provided variable name is defined in the current registry.
        [[nodiscard]]
        bool is_field_defined_in_scope(const std::string& variable_name) const;

        /// Checks whether the provided variable name is defined in the global registry.
        [[nodiscard]]
        bool is_field_defined_globally(const std::string& field_name) const;

        /// Checks whether the provided internal function name is defined in the global registry.
        /// Do note that the internal name is not the name that you would use in
        /// source code, but rather the mangled name used for code generation.
        [[nodiscard]]
        bool is_function_defined_globally(const std::string& internal_function_name) const;

        [[nodiscard]]
        bool is_symbol_type_defined_globally(const std::string& symbol_name, const SymbolType& type) const;

    private:
        [[nodiscard]]
        const SymbolRegistry& traverse_to_root() const;
    };
}
