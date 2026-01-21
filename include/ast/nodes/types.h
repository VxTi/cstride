#pragma once

#include <memory>
#include <optional>

#include "ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define SRFLAG_TYPE_PTR            (0x1)
#define SRFLAG_TYPE_REFERENCE      (0x2)
#define SRFLAG_TYPE_MUTABLE        (0x4)

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
        VOID
    };

    std::string primitive_type_to_str(PrimitiveType type);

    class AstType : public IAstNode
    {
        const int _flags;

    public:
        AstType(
            const s_ptr<SourceFile>& source,
            const int source_offset,
            const int flags
        )
            : IAstNode(source, source_offset),
              _flags(flags) {}

        ~AstType() override = default;

        [[nodiscard]]
        virtual u_ptr<AstType> clone() const = 0;

        [[nodiscard]]
        bool is_pointer() const { return this->_flags & SRFLAG_TYPE_PTR; }

        [[nodiscard]]
        bool is_reference() const { return this->_flags & SRFLAG_TYPE_REFERENCE; }

        [[nodiscard]]
        bool is_mutable() const { return this->_flags & SRFLAG_TYPE_MUTABLE; }

        [[nodiscard]]
        int get_flags() const { return this->_flags; }

        [[nodiscard]]
        virtual std::string get_internal_name() = 0;

        virtual bool operator==(AstType& other) = 0;
    };

    class AstPrimitiveType
        : public AstType
    {
        const PrimitiveType _type;
        size_t _byte_size;

    public:
        explicit AstPrimitiveType(
            const s_ptr<SourceFile>& source,
            const int source_offset,
            const PrimitiveType type,
            const size_t byte_size,
            const int flags
        ) :
            AstType(source, source_offset, flags),
            _type(type),
            _byte_size(byte_size) {}

        std::string to_string() override;

        static std::optional<u_ptr<AstPrimitiveType>> parse_primitive_type_optional(TokenSet& set);

        [[nodiscard]]
        PrimitiveType type() const { return this->_type; }

        [[nodiscard]]
        size_t byte_size() const { return this->_byte_size; }

        /* * * * * * * * Override fields * * * * * * * */

        [[nodiscard]]
        u_ptr<AstType> clone() const override
        {
            return std::make_unique<AstPrimitiveType>(*this);
        }

        std::string get_internal_name() override { return primitive_type_to_str(_type); }

        // For primitives, we can easily compare whether another is equal by comparing the `PrimitiveType` enumerable
        bool operator==(AstType& other) override
        {
            if (const auto* other_primitive = dynamic_cast<AstPrimitiveType*>(&other))
            {
                return this->_type == other_primitive->_type;
            }
            return false;
        }
    };

    class AstNamedType
        : public AstType
    {
        std::string _name;

    public:
        std::string to_string() override;

        explicit AstNamedType(
            const s_ptr<SourceFile>& source,
            const int source_offset,
            std::string name,
            const int flags
        ) :
            AstType(source, source_offset, flags),
            _name(std::move(name)) {}

        static std::optional<u_ptr<AstNamedType>> parse_named_type_optional(TokenSet& set);

        [[nodiscard]]
        std::string name() const { return this->_name; }

        [[nodiscard]]
        u_ptr<AstType> clone() const override
        {
            return std::make_unique<AstNamedType>(*this);
        }

        std::string get_internal_name() override { return this->_name; }

        bool operator==(AstType& other) override
        {
            if (const auto* other_named = dynamic_cast<AstNamedType*>(&other))
            {
                return this->_name == other_named->_name;
            }
            return false;
        }
    };

    u_ptr<AstType> parse_primitive_type(TokenSet& set);

    llvm::Type* internal_type_to_llvm_type(AstType* type, llvm::Module* module, llvm::LLVMContext& context);

    u_ptr<AstType> get_dominant_type(AstType* lhs, AstType* rhs);

    /**
     * Resolves a unique identifier (UID) for the given internal type.
     *
     * This function generates a unique identifier for the provided AstType object.
     * The UID is determined based on the specific characteristics of the type,
     * such as whether it is a primitive type, custom type, pointer, or reference.
     *
     * @param type A pointer to the AstType object for which the UID is to be resolved.
     *             The AstType can represent either a primitive or a custom type.
     * @return The unique identifier (UID) associated with the given internal type.
     */
    size_t ast_type_to_internal_id(const AstType* type);
}
