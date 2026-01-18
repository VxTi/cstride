#pragma once
#include <memory>
#include <optional>

#include "ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast::types
{
#define SRFLAG_TYPE_PTR       (0x1)
#define SRFLAG_TYPE_REFERENCE (0x2)

    enum class PrimitiveType
    {
        INT8,
        INT16,
        INT32,
        INT64,
        FLOAT32,
        FLOAT64,
        BOOL,
        CHAR,
        STRING,
        VOID
    };

    class AstType : public IAstNode
    {
        const int flags;

    public:
        AstType(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const int flags
        )
            : IAstNode(source, source_offset),
              flags(flags) {}

        ~AstType() override = default;

        virtual u_ptr<AstType> clone() const = 0;

        [[nodiscard]]
        bool is_pointer() const { return this->flags & SRFLAG_TYPE_PTR; }
    };

    class AstPrimitiveType : public AstType
    {
        const PrimitiveType _type;
        size_t _byte_size;

    public:
        explicit AstPrimitiveType(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const PrimitiveType type,
            const size_t byte_size,
            const int flags
        ) :
            AstType(source, source_offset, flags),
            _type(type),
            _byte_size(byte_size) {}

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstPrimitiveType>> try_parse(TokenSet& set);

        [[nodiscard]]
        PrimitiveType type() const { return this->_type; }

        [[nodiscard]]
        size_t byte_size() const { return this->_byte_size; }

        std::unique_ptr<AstType> clone() const override
        {
            return std::make_unique<AstPrimitiveType>(*this);
        }
    };

    class AstCustomType : public AstType
    {
        std::string _name;

    public:
        std::string to_string() override;

        explicit AstCustomType(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string name,
            const int flags
        ) :
            AstType(source, source_offset, flags),
            _name(std::move(name)) {}

        static std::optional<std::unique_ptr<AstCustomType>> try_parse(TokenSet& set);

        [[nodiscard]]
        std::string name() const { return this->_name; }

        std::unique_ptr<AstType> clone() const override
        {
            return std::make_unique<AstCustomType>(*this);
        }
    };

    std::unique_ptr<AstType> parse_primitive_type(TokenSet& set);

    llvm::Type* internal_type_to_llvm_type(AstType* type, llvm::Module* module, llvm::LLVMContext& context);
}
