#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"
#include "expression.h"

namespace stride::ast
{
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
    public:
        AstLiteral() : AstExpression({}) {}

        std::string to_string() override = 0;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse(const Scope& scope, TokenSet& tokens);
    };

    template <typename T>
    class AstLiteralBase : public AstLiteral
    {
        T _value;
        char _bit_count;

    public:
        explicit AstLiteralBase(T value, const char byte_count) :
            _value(value),
            _bit_count(byte_count) {}

        [[nodiscard]] const T& value() const { return this->_value; }

        [[nodiscard]] char bit_count() const { return this->_bit_count; }
    };

    class AstStringLiteral : public AstLiteralBase<std::string>
    {
    public:
        explicit AstStringLiteral(std::string val) :
            // Strings are only considered to be a single byte,
            // as they're pointing to a memory location
            AstLiteralBase(std::move(val), 1 * BITS_PER_BYTE) {}

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };

    class AstIntegerLiteral : public AstLiteralBase<int64_t>
    {
    public:
        explicit AstIntegerLiteral(const int64_t value) :
            AstLiteralBase(value, BITS_PER_BYTE * INFER_INT_BYTE_COUNT(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };

    class AstFloatLiteral : public AstLiteralBase<float>
    {
    public :
        explicit AstFloatLiteral(const long double value) :
            AstLiteralBase(value, 4 * BITS_PER_BYTE) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstBooleanLiteral : public AstLiteralBase<bool>
    {
    public:
        explicit AstBooleanLiteral(const bool value) :
            AstLiteralBase(value, 1 /* Single bit only*/) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstCharLiteral : public AstLiteralBase<char>
    {
    public:
        explicit AstCharLiteral(const char value) :
            AstLiteralBase(value, BITS_PER_BYTE) {}

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

    bool is_ast_literal(IAstNode *node);
}
