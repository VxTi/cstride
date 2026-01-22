#pragma once
#include <memory>
#include <string>
#include <vector>

#include "flags.h"
#include "ast/nodes/types.h"

namespace stride::ast
{
    enum class ScopeType
    {
        GLOBAL,
        FUNCTION,
        CLASS,
        BLOCK
    };

    enum class IdentifiableSymbolType
    {
        CLASS,
        VARIABLE,
        ENUM,
        ENUM_MEMBER,
        STRUCT,
        STRUCT_MEMBER
    };

    static std::string scope_type_to_str(const ScopeType& type)
    {
        switch (type)
        {
        case ScopeType::GLOBAL: return "global";
        case ScopeType::FUNCTION: return "function";
        case ScopeType::CLASS: return "class";
        case ScopeType::BLOCK: return "block";
        }
        return "unknown";
    }

    class ISymbolDef
    {
        std::string _internal_name;

    public:
        explicit ISymbolDef(const std::string& symbol_name) :
            _internal_name(symbol_name) {}

        virtual ~ISymbolDef() = default;

        std::string get_internal_symbol_name() const { return this->_internal_name; }
    };

    class IdentifiableSymbolDef
        : public ISymbolDef
    {
        IdentifiableSymbolType _type;

    public:
        explicit IdentifiableSymbolDef(
            const IdentifiableSymbolType type,
            const std::string& symbol_name
        ) : ISymbolDef(symbol_name),
            _type(type) {}

        IdentifiableSymbolType get_symbol_type() const { return this->_type; }
    };

    /// Can be either a variable or a field in a struct/class
    class FieldSymbolDef
        : public ISymbolDef
    {
        std::shared_ptr<IAstInternalFieldType> _type;
        std::string _variable_name;
        bool is_mutable;

    public:
        explicit FieldSymbolDef(
            const std::string& field_name,
            const std::string& internal_name,
            std::shared_ptr<IAstInternalFieldType> type,
            const int flags
        ) : ISymbolDef(internal_name),
            _type(std::move(type)),
            _variable_name(field_name),
            is_mutable(flags & SRFLAG_VAR_MUTABLE) {}

        IAstInternalFieldType* get_type() const { return this->_type.get(); }

        const std::string& get_variable_name() const { return this->_variable_name; }
    };

    class SymbolFnDefinition
        : public ISymbolDef
    {
        std::vector<std::shared_ptr<IAstInternalFieldType>> _parameter_types;
        std::shared_ptr<IAstInternalFieldType> _return_type;

    public:
        explicit SymbolFnDefinition(
            std::vector<std::shared_ptr<IAstInternalFieldType>> parameter_types,
            std::shared_ptr<IAstInternalFieldType> return_type,
            const std::string& internal_name
        ) :
            ISymbolDef(internal_name),
            _parameter_types(std::move(parameter_types)),
            _return_type(std::move(return_type)) {}

        ~SymbolFnDefinition() override;

        std::vector<const IAstInternalFieldType*> get_parameter_types() const
        {
            std::vector<const IAstInternalFieldType*> out;
            out.reserve(this->_parameter_types.size());
            for (const auto& p : this->_parameter_types)
                out.push_back(p.get());
            return out;
        }

        const IAstInternalFieldType* get_return_type() const { return this->_return_type.get(); }
    };

    class Scope;

    class Scope
    {
    public:
        ScopeType _type;
        std::shared_ptr<Scope> parent_scope;

        std::vector<std::unique_ptr<ISymbolDef>> symbols = {};

        explicit Scope(
            std::shared_ptr<Scope> parent,
            const ScopeType type
        ) : _type(type),
            parent_scope(std::move(parent)) {}

        explicit Scope(const ScopeType type)
            : Scope(nullptr, type) {}

        ScopeType get_scope_type() const { return this->_type; }

        FieldSymbolDef* get_variable_def(const std::string& variable_name) const;

        SymbolFnDefinition* get_function_def(const std::string& function_name) const;

        IdentifiableSymbolDef* get_symbol_def(const std::string& symbol_name) const;

        /// Will attempt to define the function in the global scope.
        void define_function(
            const std::string& internal_function_name,
            std::vector<std::shared_ptr<IAstInternalFieldType>> parameter_types,
            std::shared_ptr<IAstInternalFieldType> return_type
        );

        void define_field(
            const std::string& field_name,
            const std::string& internal_name,
            const std::shared_ptr<IAstInternalFieldType>& type,
            int flags
        );

        void define_symbol(const std::string& symbol_name, IdentifiableSymbolType type);

        /// Checks whether the provided variable name is defined in the current scope.
        bool is_variable_defined_in_scope(const std::string& variable_name) const;

        /// Checks whether the provided variable name is defined in the global scope.
        bool is_variable_defined_globally(const std::string& variable_name) const;

        /// Checks whether the provided internal function name is defined in the global scope.
        /// Do note that the internal name is not the name that you would use in
        /// source code, but rather the mangled name used for code generation.
        bool is_function_defined_globally(const std::string& internal_function_name);

        bool is_symbol_type_defined_globally(const std::string& symbol_name, const IdentifiableSymbolType& type);

    private:
        std::shared_ptr<Scope> traverse_to_root();
    };
}
