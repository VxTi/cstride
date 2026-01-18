#pragma once
#include <memory>
#include <optional>

#include "ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast::types
{
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
        bool is_ptr;

    public:
        AstType(const bool is_ptr = false) : is_ptr(is_ptr) {}
        ~AstType() override = default;

        virtual u_ptr<AstType> clone() const = 0;

        [[nodiscard]]
        bool is_pointer() const { return this->is_ptr; }
    };

    class AstPrimitiveType : public AstType
    {
        const PrimitiveType _type;
        size_t _byte_size;

    public:
        explicit AstPrimitiveType(const PrimitiveType type, const size_t byte_size, const bool is_ptr = false) :
            AstType(is_ptr),
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

        explicit AstCustomType(std::string name, const bool is_ptr) :
            AstType(is_ptr),
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
