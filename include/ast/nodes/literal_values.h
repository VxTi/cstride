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
    ((x > 0xFFFFFFFF) ? 8 : 4)

#define INFER_FLOAT_BYTE_COUNT(x) \
    ((x >> 23) & 0xFF) ? 4 : \
    ((x >> 12) & 0xFF) ? 2 : \
    ((x >> 6) & 0xFF) ? 1 : 0

#define SRFLAG_INT_SIGNED   (0)
#define SRFLAG_INT_UNSIGNED (1)


    class AstLiteral :
        public AstExpression
    {
        char _bit_count;
        LiteralType _type;

    public:
        AstLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const LiteralType type,
            const char byte_count
        )
            : AstExpression(source, source_offset, {}),
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
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const LiteralType type,
            const T value,
            const char byte_count
        ) :
            AstLiteral(source, source_offset, type, byte_count),
            _value(value) {}

        [[nodiscard]]
        const T& value() const { return this->_value; }
    };

    class AstStringLiteral : public AbstractAstLiteralBase<std::string>
    {
    public:
        explicit AstStringLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string val
        ) :
            // Strings are only considered to be a single byte,
            // as they're pointing to a memory location
            AbstractAstLiteralBase(source, source_offset, LiteralType::STRING, std::move(val), 1 * BITS_PER_BYTE) {}

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstIntegerLiteral : public AbstractAstLiteralBase<int64_t>
    {
        const int flags;

    public:
        explicit AstIntegerLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const int64_t value,
            const int flags = SRFLAG_INT_SIGNED
        ) :
            AbstractAstLiteralBase(source, source_offset, LiteralType::INTEGER, value,
                                   BITS_PER_BYTE * INFER_INT_BYTE_COUNT(value)),
            flags(flags) {}

        int get_flags() const { return this->flags; }

        bool is_signed() const { return this->flags & SRFLAG_INT_SIGNED; }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstFloatLiteral : public AbstractAstLiteralBase<float>
    {
    public :
        explicit AstFloatLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const long double value
        ) :
            AbstractAstLiteralBase(source, source_offset, LiteralType::FLOAT, value, 8 * BITS_PER_BYTE) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstBooleanLiteral : public AbstractAstLiteralBase<bool>
    {
    public:
        explicit AstBooleanLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const bool value
        ) :
            AbstractAstLiteralBase(source, source_offset, LiteralType::BOOLEAN, value, 1 /* Single bit only*/) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    class AstCharLiteral : public AbstractAstLiteralBase<char>
    {
    public:
        explicit AstCharLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const char value
        ) :
            AbstractAstLiteralBase(source, source_offset, LiteralType::CHAR, value, BITS_PER_BYTE) {}

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    std::optional<std::unique_ptr<AstLiteral>> parse_literal_optional(const Scope& scope, TokenSet& tokens);

    std::optional<std::unique_ptr<AstLiteral>> parse_boolean_literal_optional(
        const Scope& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_float_literal_optional(
        const Scope& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_integer_literal_optional(
        const Scope& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_string_literal_optional(
        const Scope& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_char_literal_optional(
        const Scope& scope,
        TokenSet& set
    );

    bool is_ast_literal(IAstNode* node);
}
