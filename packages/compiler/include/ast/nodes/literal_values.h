#pragma once

#include "ast/nodes/ast_node.h"
#include "ast/nodes/expression.h"

#include <utility>

#define BITS_PER_BYTE (8)
#define INFER_INT_BIT_COUNT(x) (((x) > 0xFFFFFFFF) ? ((short)64) : ((short)32))

namespace stride::ast
{
    class AstLiteral
        : public IAstExpression
    {
        PrimitiveType _primitive_type;

    public:
        explicit AstLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const PrimitiveType type
        ) :
            IAstExpression(source, context),
            _primitive_type(type) {}

        ~AstLiteral() override = default;

        std::string to_string() override = 0;

        [[nodiscard]]
        size_t get_bit_count() const
        {
            return get_primitive_bit_count(this->_primitive_type);
        }

        [[nodiscard]]
        PrimitiveType get_primitive_type() const
        {
            return this->_primitive_type;
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
            const PrimitiveType type,
            T value
        ) :
            AstLiteral(source, context, type),
            _value(std::move(value)) {}

        [[nodiscard]]
        const T& value() const
        {
            return this->_value;
        }
    };

    class AstStringLiteral
        : public IAstLiteralBase<std::string>
    {
    public:
        explicit AstStringLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string val
        ) :
            // Strings are only considered to be a single byte,
            // as they're pointing to a memory location
            IAstLiteralBase(source, context, PrimitiveType::STRING, std::move(val)) {}

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstIntLiteral
        : public IAstLiteralBase<int64_t>
    {
        const int _flags;

    public:
        explicit AstIntLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const PrimitiveType type,
            const int64_t value,
            const int flags = SRFLAG_TYPE_INT_SIGNED
        ) :
            IAstLiteralBase(source, context, type, value),
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
            llvm::IRBuilderBase* builder) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstFpLiteral
        : public IAstLiteralBase<long double>
    {
    public:
        explicit AstFpLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const PrimitiveType type,
            const long double value
        ) :
            IAstLiteralBase(source, context, type, value) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstBooleanLiteral
        : public IAstLiteralBase<bool>
    {
    public:
        explicit AstBooleanLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const bool value
        ) :
            IAstLiteralBase(source, context, PrimitiveType::BOOL, value) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstCharLiteral : public IAstLiteralBase<char>
    {
    public:
        explicit AstCharLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const char value
        ) :
            IAstLiteralBase(source, context, PrimitiveType::CHAR, value) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstNilLiteral
        : public AstLiteral
    {
    public:
        AstNilLiteral(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context
        ) :
            AstLiteral(source, context, PrimitiveType::NIL) {}

        std::string to_string() override
        {
            return "nil";
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    std::optional<std::unique_ptr<AstLiteral>> parse_literal_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

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

    inline bool is_literal_ast_node(IAstNode* node)
    {
        return dynamic_cast<AstLiteral*>(node) != nullptr;
    }
} // namespace stride::ast
