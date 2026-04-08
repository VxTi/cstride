#pragma once
#include "ast/symbols.h"
#include "ast/nodes/types.h"

namespace stride::ast::definition
{
    class FunctionDefinition;

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
        VisibilityModifier _visibility;

    public:
        explicit IDefinition(
            Symbol symbol,
            const VisibilityModifier modifier
        ) :
            _symbol(std::move(symbol)),
            _visibility(modifier) {}

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

        [[nodiscard]]
        VisibilityModifier get_visibility() const
        {
            return this->_visibility;
        }

        [[nodiscard]]
        virtual std::unique_ptr<IDefinition> clone() const = 0;

        void set_visibility(const VisibilityModifier visibility)
        {
            this->_visibility = visibility;
        }
    };

    class TypeDefinition
        : public Cloneable<TypeDefinition>
    {
        std::unique_ptr<IAstType> _type;
        GenericParameterList _generics;
        VisibilityModifier _visibility;
        Symbol _type_name_symbol;

    public:
        explicit TypeDefinition(
            Symbol type_name_symbol,
            std::unique_ptr<IAstType> type,
            GenericParameterList generics,
            const VisibilityModifier visibility
        ) :
            _type(std::move(type)),
            _generics(std::move(generics)),
            _visibility(visibility),
            _type_name_symbol(std::move(type_name_symbol)) {}

        [[nodiscard]]
        std::unique_ptr<IAstType> get_type() const
        {
            return this->_type->clone();
        }

        [[nodiscard]]
        IAstType* get_type_ptr() const
        {
            return this->_type.get();
        }

        [[nodiscard]]
        GenericParameterList get_generics_parameters() const
        {
            return this->_generics;
        }

        [[nodiscard]]
        bool is_generic() const
        {
            return !this->_generics.empty();
        }

        [[nodiscard]]
        std::unique_ptr<TypeDefinition> clone() const
        {
            return std::make_unique<TypeDefinition>(
                this->_type_name_symbol,
                _type->clone(),
                get_generics_parameters(),
                this->_visibility
            );
        }

        [[nodiscard]]
        Symbol get_type_name_symbol() const
        {
            return this->_type_name_symbol;
        }
    };

    class FieldDefinition : public IDefinition
    {
        std::unique_ptr<IAstType> _type;
        int _flags;

        /// Can be either a variable or a field in a struct/class
    public:
        explicit FieldDefinition(
            const Symbol& symbol,
            std::unique_ptr<IAstType> type,
            const VisibilityModifier visibility,
            const int flags = 0
        ) :
            IDefinition(symbol, visibility),
            _type(std::move(type)),
            _flags(flags) {}

        explicit FieldDefinition(
            const Symbol& symbol,
            const VisibilityModifier visibility,
            const int flags = 0
        ) :
            FieldDefinition(symbol, nullptr, visibility, flags) {}

        [[nodiscard]]
        IAstType* get_type() const
        {
            if (!this->_type)
                return nullptr;

            return this->_type.get();
        }

        [[nodiscard]]
        std::string get_field_name() const
        {
            return this->get_symbol().name;
        }

        [[nodiscard]]
        std::unique_ptr<IDefinition> clone() const override
        {
            return std::make_unique<FieldDefinition>(get_symbol(), _type->clone(), get_visibility());
        }

        void set_type(std::unique_ptr<IAstType> type)
        {
            this->_type = std::move(type);
        }

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }
    };
} // namespace stride::ast::definition
