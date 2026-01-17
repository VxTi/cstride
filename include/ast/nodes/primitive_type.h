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
    public:
        AstType() = default;
        ~AstType() override = default;
    };

    class AstPrimitiveType : public AstType
    {
        const PrimitiveType _type;

    public:
        size_t byte_size;

        explicit AstPrimitiveType(const PrimitiveType type, const size_t byte_size) :
            _type(type),
            byte_size(byte_size) {}

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstPrimitiveType>> try_parse(TokenSet& set);

        [[nodiscard]]
        PrimitiveType type() const { return this->_type; }
    };

    class AstCustomType : public AstType
    {
        std::string _name;

    public:
        std::string to_string() override;

        explicit AstCustomType(std::string name) : _name(std::move(name)) {}

        static std::optional<std::unique_ptr<AstCustomType>> try_parse(TokenSet& set);

        [[nodiscard]]
        std::string name() const { return this->_name; }
    };

    std::unique_ptr<AstType> parse_primitive_type(TokenSet& set);

    llvm::Type* internal_type_to_llvm_type(AstType* type, llvm::Module* module, llvm::LLVMContext& context);
}
