#pragma once

#include <memory>
#include <optional>

#include "ast_node.h"
#include "ast/flags.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    enum class PrimitiveType
    {
        INT8,
        INT16,
        INT32,
        INT64,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        FLOAT32,
        FLOAT64,
        BOOL,
        CHAR,
        STRING,
        VOID,
        // Reserved type for empty arrays;
        // It's impossible to deduce the type from an empty array,
        // as it would otherwise be done by inferring the type of its members.
        // Therefore, we'll leave it as a special case.
        UNKNOWN
    };

    std::string primitive_type_to_str(PrimitiveType type);

    class IAstInternalFieldType :
        public IAstNode
    {
        int _flags;

    public:
        IAstInternalFieldType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            const int flags
        )
            : IAstNode(source, source_position, scope),
              _flags(flags) {}

        ~IAstInternalFieldType() override = default;

        [[nodiscard]]
        virtual std::unique_ptr<IAstInternalFieldType> clone() const = 0;

        [[nodiscard]]
        bool is_pointer() const { return this->_flags & SRFLAG_TYPE_PTR; }

        [[nodiscard]]
        bool is_reference() const { return this->_flags & SRFLAG_TYPE_REFERENCE; }

        [[nodiscard]]
        bool is_mutable() const { return this->_flags & SRFLAG_TYPE_MUTABLE; }

        [[nodiscard]]
        bool is_optional() const { return this->_flags & SRFLAG_TYPE_OPTIONAL; }

        [[nodiscard]]
        bool is_global() const { return this->_flags & SRFLAG_TYPE_GLOBAL; }

        [[nodiscard]]
        bool is_variadic() const { return this->_flags & SRFLAG_TYPE_VARIADIC; }

        [[nodiscard]]
        int get_flags() const { return this->_flags; }

        void set_flags(const int flags)
        {
            this->_flags = flags;
        }

        [[nodiscard]]
        virtual std::string get_internal_name() = 0;

        virtual bool operator==(IAstInternalFieldType& other) = 0;

        virtual bool operator!=(IAstInternalFieldType& other) = 0;
    };

    class AstPrimitiveFieldType
        : public IAstInternalFieldType
    {
        PrimitiveType _type;
        size_t _byte_size;

    public:
        explicit AstPrimitiveFieldType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            const PrimitiveType type,
            const size_t byte_size,
            const int flags = SRFLAG_NONE
        ) :
            IAstInternalFieldType(source, source_position, scope, flags),
            _type(type),
            _byte_size(byte_size) {}

        ~AstPrimitiveFieldType() override = default;

        [[nodiscard]]
        PrimitiveType type() const { return this->_type; }

        [[nodiscard]]
        bool is_integer_ty() const
        {
            switch (this->_type)
            {
            case PrimitiveType::INT8:
            case PrimitiveType::INT16:
            case PrimitiveType::INT32:
            case PrimitiveType::INT64:
            case PrimitiveType::UINT8:
            case PrimitiveType::UINT16:
            case PrimitiveType::UINT32:
            case PrimitiveType::UINT64:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]]
        bool is_fp() const
        {
            switch (this->_type)
            {
            case PrimitiveType::FLOAT32:
            case PrimitiveType::FLOAT64:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]]
        size_t byte_size() const { return this->_byte_size; }

        [[nodiscard]]
        std::unique_ptr<IAstInternalFieldType> clone() const override
        {
            return std::make_unique<AstPrimitiveFieldType>(*this);
        }

        std::string get_internal_name() override { return primitive_type_to_str(_type); }

        std::string to_string() override
        {
            return std::format("{}{}", primitive_type_to_str(this->type()), this->is_pointer() ? "*" : "");
        }

        // For primitives, we can easily compare whether another is equal by comparing the `PrimitiveType` enumerable
        bool operator==(IAstInternalFieldType& other) override
        {
            if (const auto* other_primitive = dynamic_cast<AstPrimitiveFieldType*>(&other))
            {
                return this->type() == other_primitive->type();
            }
            return false;
        }

        bool operator!=(IAstInternalFieldType& other) override
        {
            return !(*this == other);
        }
    };

    class AstStructType
        : public IAstInternalFieldType
    {
        std::string _name;

    public:
        explicit AstStructType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::string name,
            const int flags = SRFLAG_NONE
        ) :
            IAstInternalFieldType(source, source_position, scope, flags),
            _name(std::move(name)) {}

        [[nodiscard]]
        std::string name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        std::unique_ptr<IAstInternalFieldType> clone() const override
        {
            return std::make_unique<AstStructType>(*this);
        }

        std::string get_internal_name() override { return this->_name; }

        std::string to_string() override
        {
            return std::format("{}{}", this->name(), this->is_pointer() ? "*" : "");
        }

        bool operator==(IAstInternalFieldType& other) override
        {
            if (const auto* other_named = dynamic_cast<AstStructType*>(&other))
            {
                return this->_name == other_named->_name;
            }
            return false;
        }

        bool operator!=(IAstInternalFieldType& other) override
        {
            return !(*this == other);
        }
    };

    class AstArrayType
        : public IAstInternalFieldType
    {
        std::unique_ptr<IAstInternalFieldType> _element_type;
        size_t _initial_length;

    public:
        explicit AstArrayType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::unique_ptr<IAstInternalFieldType> element_type,
            const size_t initial_length
        ) : IAstInternalFieldType(
                source,
                source_position,
                scope,
                (element_type ? element_type->get_flags() : 0) | SRFLAG_TYPE_PTR
            ),
            // Arrays are always ptrs
            _element_type(std::move(element_type)),
            _initial_length(initial_length) {}

        [[nodiscard]]
        std::unique_ptr<IAstInternalFieldType> clone() const override
        {
            auto element_clone = this->_element_type
                                     ? this->_element_type->clone()
                                     : nullptr;

            return std::make_unique<AstArrayType>(
                this->get_source(),
                this->get_source_position(),
                this->get_registry(),
                std::move(element_clone),
                this->_initial_length
            );
        }

        IAstInternalFieldType* get_element_type() const { return this->_element_type.get(); }

        [[nodiscard]]
        size_t get_initial_length() const { return this->_initial_length; }

        std::string to_string() override
        {
            return std::format("Array[{}]", this->_element_type->to_string());
        }

        [[nodiscard]]
        std::string get_internal_name() override
        {
            return std::format("[{}]", this->_element_type->get_internal_name());
        }

        bool operator==(IAstInternalFieldType& other) override
        {
            if (const auto* other_array = dynamic_cast<AstArrayType*>(&other))
            {
                // Length here doesn't matter; merely the types should be the same.
                return *this->_element_type == *other_array->_element_type;
            }
            return false;
        }

        bool operator!=(IAstInternalFieldType& other) override
        {
            return !(*this == other);
        }
    };


    std::unique_ptr<IAstInternalFieldType> parse_type(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set,
        const std::string& error,
        int context_flags = SRFLAG_NONE
    );

    llvm::Type* internal_type_to_llvm_type(
        IAstInternalFieldType* type,
        llvm::Module* module
    );

    std::unique_ptr<IAstInternalFieldType> get_dominant_field_type(
        const std::shared_ptr<SymbolRegistry>& scope,
        IAstInternalFieldType* lhs,
        IAstInternalFieldType* rhs
    );

    /**
     * Resolves a unique identifier (UID) for the given internal type.
     *
     * This function generates a unique identifier for the provided IAstInternalFieldType object.
     * The UID is determined based on the specific characteristics of the type,
     * such as whether it is a primitive type, custom type, pointer, or reference.
     *
     * @param type A pointer to the AstType object for which the UID is to be resolved.
     *             The AstType can represent either a primitive or a custom type.
     * @return The unique identifier (UID) associated with the given internal type.
     */
    size_t ast_type_to_internal_id(IAstInternalFieldType* type);

    std::optional<std::unique_ptr<IAstInternalFieldType>> parse_primitive_type_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set,
        int context_type_flags = SRFLAG_NONE
    );

    std::optional<std::unique_ptr<IAstInternalFieldType>> parse_named_type_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set,
        int context_type_flags = SRFLAG_NONE
    );
}
