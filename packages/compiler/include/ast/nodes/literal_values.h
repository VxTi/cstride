#pragma once

#include "ast/nodes/ast_node.h"
#include "ast/nodes/expression.h"

#include <utility>

#define BITS_PER_BYTE (8)
#define INFER_INT_BIT_COUNT(x) (((x) > 0xFFFFFFFF) ? ((short)64) : ((short)32))

#define INFER_FLOAT_BYTE_COUNT(x)                                                                  \
    (((x) >> 23) & 0xFF) ? 4 : (((x) >> 12) & 0xFF) ? 2 : (((x) >> 6) & 0xFF) ? 1 : 0

#define SRFLAG_INT_UNSIGNED (0x2)

namespace stride::ast
{
    enum class LiteralType
    {
        STRING,
        INTEGER,
        FLOAT,
        BOOLEAN,
        CHAR,
        NIL
    };

    class AstLiteral
        : public AstExpression
    {
        short _bit_count;
        LiteralType _type;

    public:
        AstLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const LiteralType type,
            const short bit_count) :
            AstExpression(source, context),
            _bit_count(bit_count),
            _type(type) {}

        ~AstLiteral() override = default;

        std::string to_string() override = 0;

        [[nodiscard]]
        short bit_count() const
        {
            return this->_bit_count;
        }

        [[nodiscard]]
        LiteralType get_type() const
        {
            return this->_type;
        }
    };

    template <typename T>
    class IAstLiteralBase
        : public AstLiteral
    {
        T _value;

    public:
        explicit IAstLiteralBase(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const LiteralType type,
            T value,
            const short bit_count) :
            AstLiteral(source, context, type, bit_count),
            _value(std::move(value)) {}

        [[nodiscard]]
        const T& value() const
        {
            return this->_value;
        }
    };

    class AstStringLiteral : public IAstLiteralBase<std::string>
    {
    public:
        explicit AstStringLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string val) :
            // Strings are only considered to be a single byte,
            // as they're pointing to a memory location
            IAstLiteralBase(source,
                            context,
                            LiteralType::STRING,
                            std::move(val),
                            1) {}

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    class AstIntLiteral : public IAstLiteralBase<int64_t>
    {
        const int _flags;

    public:
        explicit AstIntLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const int64_t value,
            const short bit_count,
            const int flags = SRFLAG_TYPE_INT_SIGNED) :
            IAstLiteralBase(source,
                            context,
                            LiteralType::INTEGER,
                            value,
                            bit_count),
            _flags(flags) {}

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }

        [[nodiscard]]
        bool is_signed() const
        {
            return this->_flags & SRFLAG_TYPE_INT_SIGNED;
        }

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    class AstFpLiteral : public IAstLiteralBase<long double>
    {
    public:
        explicit AstFpLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const long double value,
            const short bit_count) :
            IAstLiteralBase(source,
                            context,
                            LiteralType::FLOAT,
                            value,
                            bit_count) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    class AstBooleanLiteral : public IAstLiteralBase<bool>
    {
    public:
        explicit AstBooleanLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const bool value) :
            IAstLiteralBase(
                source,
                context,
                LiteralType::BOOLEAN,
                value,
                1 /* Single bit only*/
            ) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    class AstCharLiteral : public IAstLiteralBase<char>
    {
    public:
        explicit AstCharLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const char value) :
            IAstLiteralBase(source,
                            context,
                            LiteralType::CHAR,
                            value,
                            8) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    class AstNilLiteral : public AstLiteral
    {
    public:
        AstNilLiteral(const SourceFragment& source,
                      const std::shared_ptr<ParsingContext>& context) :
            AstLiteral(source, context, LiteralType::NIL, 8) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;
    };

    std::optional<std::unique_ptr<AstLiteral>> parse_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& tokens);

    std::optional<std::unique_ptr<AstLiteral>> parse_boolean_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    std::optional<std::unique_ptr<AstLiteral>> parse_float_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    std::optional<std::unique_ptr<AstLiteral>> parse_integer_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    std::optional<std::unique_ptr<AstLiteral>> parse_string_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    std::optional<std::unique_ptr<AstLiteral>> parse_char_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    bool is_literal_ast_node(IAstNode* node);
} // namespace stride::ast
