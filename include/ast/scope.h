#pragma once
#include <memory>
#include <string>
#include <optional>
#include <vector>

#include "files.h"
#include "identifiers.h"
#include "tokens/token.h"
#include "ast/nodes/functions.h"
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

    enum class SymbolType
    {
        VARIABLE,
        FUNCTION,
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

    class InternalSymbolDefinition
    {
        SymbolType _symbol_type;

    public:
        explicit InternalSymbolDefinition(const SymbolType symbol_type) : _symbol_type(symbol_type) {}
        virtual ~InternalSymbolDefinition() = default;

        SymbolType get_symbol_type() const { return this->_symbol_type; }
    };

    class SymbolDefinition
        : public InternalSymbolDefinition
    {
        std::string _symbol;
        const Token& _reference_token;
        std::string _internal_name;

    public:
        explicit SymbolDefinition(
            const std::string& symbol,
            const Token& reference_token,
            const SymbolType symbol_type,
            std::string internal_name = ""
        ) : InternalSymbolDefinition(symbol_type),
            _symbol(symbol),
            _reference_token(reference_token),
            _internal_name(std::move(internal_name)) {}


        const std::string& get_symbol() const { return this->_symbol; }

        const Token& get_reference_token() const { return this->_reference_token; }

        const std::string& get_internal_name() const { return this->_internal_name.empty() ? _symbol : _internal_name; }
    };

    class SymbolFnDefinition
        : public InternalSymbolDefinition
    {
        std::vector<std::shared_ptr<IAstInternalFieldType>> _parameter_types;
        std::shared_ptr<IAstInternalFieldType> _return_type;
        std::string _fn_name;
        std::string _internal_name;
        const Token& _reference_token;

    public:
        explicit SymbolFnDefinition(
            const std::string& function_name,
            const Token& reference_token,
            std::vector<std::shared_ptr<IAstInternalFieldType>> parameter_types,
            std::shared_ptr<IAstInternalFieldType> return_type,
            std::string internal_name
        ) :
            InternalSymbolDefinition(SymbolType::FUNCTION),
            _fn_name(function_name),
            _reference_token(reference_token),
            _parameter_types(std::move(parameter_types)),
            _return_type(std::move(return_type)),
            _internal_name(std::move(internal_name)) {}

        std::vector<std::shared_ptr<IAstInternalFieldType>> get_parameter_types() const
        {
            return this->_parameter_types;
        }

        const IAstInternalFieldType* get_return_type() const { return this->_return_type.get(); }

        const std::string& get_internal_name() const
        {
            return this->_internal_name.empty() ? this->_fn_name : this->_internal_name;
        }
    };

    class Scope;

    class Scope
    {
    public:
        ScopeType type;
        std::shared_ptr<Scope> parent_scope;
        std::unique_ptr<std::vector<SymbolDefinition>> symbols;

        Scope(
            std::shared_ptr<Scope>&& parent,
            const ScopeType type
        )
            : type(type),
              parent_scope(std::move(parent)),
              symbols(std::make_unique<std::vector<SymbolDefinition>>()) {}

        explicit Scope(const ScopeType type)
            : Scope(nullptr, type) {}

        explicit Scope(const Scope& parent, const ScopeType scope)
            : Scope(
                std::shared_ptr<Scope>(const_cast<Scope*>(&parent), [](Scope*) {}),
                scope
            ) {}

        std::optional<SymbolDefinition> get_symbol_globally(const std::string& symbol) const;

        /**
         * Searches for a symbol only in the current scope, not in parent scopes.
         * @param symbol Symbol to search for
         * @return SymbolDefinition if found in the current scope, std::nullopt otherwise
         */
        std::optional<SymbolDefinition> get_symbol(const std::string& symbol) const;

        /**
         * Defines a symbol in this scope after checking it doesn't exist globally.
         * @param source Source file for error reporting
         * @param reference_token Token representing the symbol for error reporting
         * @param symbol Symbol to define
         * @param symbol_type Type of the symbol being defined
         * @param internal_name Unique name for codegen
         * @throws parsing_error if the symbol already exists in this scope or any parent scope
         */
        void try_define_global_symbol(const SourceFile& source, const Token& reference_token, const std::string& symbol,
                                      SymbolType symbol_type, const std::string& internal_name = "") const;

        /**
         * Defines a symbol in this scope after checking it doesn't exist in the current scope only.
         * @param source Source file for error reporting
         * @param token Token representing the symbol for error reporting
         * @param symbol Symbol to define
         * @param symbol_type Type of the symbol being defined
         * @param internal_name Unique name for codegen
         * @throws parsing_error if the symbol already exists in this scope (ignores parent scopes)
         */
        void try_define_scoped_symbol(const SourceFile& source, const Token& token, const std::string& symbol,
                                      SymbolType symbol_type, const std::string& internal_name = "") const;

        /**
         * Checks if a symbol exists in this scope or any parent scope.
         * @param symbol Symbol to check
         * @return true if the symbol exists globally, false otherwise
         */
        bool is_symbol_defined_globally(const std::string& symbol) const;

        /**
         * Checks if a symbol exists only in the current scope.
         * @param symbol Symbol to check
         * @return true if the symbol exists in the current scope, false otherwise
         */
        [[nodiscard]]
        bool is_symbol_defined_scoped(const std::string& symbol) const;

        bool is_function_defined_globally(const std::string& internal_name) const;

        void try_define_function_symbol(
            const SourceFile& source,
            const Token& reference_token,
            const std::string& symbol,
            const std::vector<unique_ptr<IAstInternalFieldType>>& parameter_types,
            const shared_ptr<IAstInternalFieldType>& return_type,
            bool anonymous = false
        );
    };
}
