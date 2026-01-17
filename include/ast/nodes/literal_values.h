#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"
#include "expression.h"

namespace stride::ast
{
    enum class LiteralType
    {
        STRING,
        INTEGER,
        FLOAT,
        BOOLEAN,
        CHAR
    };

#define BITS_PER_BYTE (8)
#define INFER_INT_BYTE_COUNT(x) \
    ((x >> 48) & 0xFF) ? 8 : \
    ((x >> 32) & 0xFF) ? 4 : \
    ((x >> 16) & 0xFF) ? 2 : \
    ((x >> 8) & 0xFF) ? 1 : 0

#define INFER_FLOAT_BYTE_COUNT(x) \
    ((x >> 23) & 0xFF) ? 4 : \
    ((x >> 12) & 0xFF) ? 2 : \
    ((x >> 6) & 0xFF) ? 1 : 0

    class AstLiteral :
        public AstExpression
    {
        char _bit_count;
        LiteralType _type;

    public:
        AstLiteral(
            const LiteralType type,
            const char byte_count
        )
            : AstExpression({}),
              _bit_count(byte_count),
              _type(type) {}

        ~AstLiteral() override = default;

        std::string to_string() override = 0;

        [[nodiscard]]
        char bit_count() const { return this->_bit_count; }

        [[nodiscard]]
        LiteralType type() const { return this->_type; }
    };

    template <typename T>
    class AbstractAstLiteralBase : public AstLiteral
    {
        T _value;

    public:
        explicit AbstractAstLiteralBase(
            const LiteralType type,
            const T value,
            const char byte_count
        ) :
            AstLiteral(type, byte_count),
            _value(value) {}

        [[nodiscard]]
        const T& value() const { return this->_value; }
    };

    class AstStringLiteral : public AbstractAstLiteralBase<std::string>
    {
    public:
        explicit AstStringLiteral(std::string val) :
            // Strings are only considered to be a single byte,
            // as they're pointing to a memory location
            AbstractAstLiteralBase(LiteralType::STRING, std::move(val), 1 * BITS_PER_BYTE) {}

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };

    class AstIntegerLiteral : public AbstractAstLiteralBase<int64_t>
    {
    public:
        explicit AstIntegerLiteral(const int64_t value) :
            AbstractAstLiteralBase(LiteralType::INTEGER, value, BITS_PER_BYTE * INFER_INT_BYTE_COUNT(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };

    class AstFloatLiteral : public AbstractAstLiteralBase<float>
    {
    public :
        explicit AstFloatLiteral(const long double value) :
            AbstractAstLiteralBase(LiteralType::FLOAT, value, 4 * BITS_PER_BYTE) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstBooleanLiteral : public AbstractAstLiteralBase<bool>
    {
    public:
        explicit AstBooleanLiteral(const bool value) :
            AbstractAstLiteralBase(LiteralType::BOOLEAN, value, 1 /* Single bit only*/) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstCharLiteral : public AbstractAstLiteralBase<char>
    {
    public:
        explicit AstCharLiteral(const char value) :
            AbstractAstLiteralBase(LiteralType::CHAR, value, BITS_PER_BYTE) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    std::optional<std::unique_ptr<AstLiteral>> parse_literal_optional(const Scope& scope, TokenSet& tokens);

    std::optional<std::unique_ptr<AstLiteral>> parse_boolean_literal_optional(
        const Scope& scope,
        TokenSet& tokens
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_float_literal_optional(
        const Scope& scope,
        TokenSet& tokens
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_integer_literal_optional(
        const Scope& scope,
        TokenSet& tokens
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_string_literal_optional(
        const Scope& scope,
        TokenSet& tokens
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_char_literal_optional(
        const Scope& scope,
        TokenSet& tokens
    );

    bool is_ast_literal(IAstNode* node);
}
