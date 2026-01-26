#pragma once

#include <utility>

#include "ast/nodes/expression.h"
#include "ast/symbol_registry.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

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
#define INFER_INT_BIT_COUNT(x) \
    ((x > 0xFFFFFFFF) ? 64 : 32)

#define INFER_FLOAT_BYTE_COUNT(x) \
    ((x >> 23) & 0xFF) ? 4 : \
    ((x >> 12) & 0xFF) ? 2 : \
    ((x >> 6) & 0xFF) ? 1 : 0

#define SRFLAG_INT_SIGNED   (0)
#define SRFLAG_INT_UNSIGNED (1)


    class AstLiteral :
        public AstExpression
    {
        short _bit_count;
        LiteralType _type;

    public:
        AstLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            const LiteralType type,
            const short bit_count
        )
            : AstExpression(source, source_offset, scope),
              _bit_count(bit_count),
              _type(type) {}

        ~AstLiteral() override = default;

        std::string to_string() override = 0;

        [[nodiscard]]
        short bit_count() const { return this->_bit_count; }

        [[nodiscard]]
        LiteralType get_type() const { return this->_type; }
    };

    template <typename T>
    class AbstractAstLiteralBase : public AstLiteral
    {
        T _value;

    public:
        explicit AbstractAstLiteralBase(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            const LiteralType type,
            T value,
            const short bit_count
        ) :
            AstLiteral(source, source_offset, scope, type, bit_count),
            _value(std::move(value)) {}

        [[nodiscard]]
        const T& value() const { return this->_value; }
    };

    class AstStringLiteral : public AbstractAstLiteralBase<std::string>
    {
    public:
        explicit AstStringLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::string val
        ) :
            // Strings are only considered to be a single byte,
            // as they're pointing to a memory location
            AbstractAstLiteralBase(
                source,
                source_offset,
                scope,
                LiteralType::STRING,
                std::move(val),
                1
            ) {}

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                             llvm::LLVMContext& context,
                             llvm::IRBuilder<>* builder) override;
    };

    class AstIntLiteral : public AbstractAstLiteralBase<int64_t>
    {
        const int _flags;

    public:
        explicit AstIntLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            const int64_t value,
            const short bit_count,
            const int flags = SRFLAG_INT_SIGNED
        ) :
            AbstractAstLiteralBase(
                source,
                source_offset,
                scope,
                LiteralType::INTEGER,
                value,
                bit_count
            ),
            _flags(flags) {}

        [[nodiscard]]
        int get_flags() const { return this->_flags; }

        [[nodiscard]]
        bool is_signed() const { return this->_flags & SRFLAG_INT_SIGNED; }

        std::string to_string() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                             llvm::LLVMContext& context,
                             llvm::IRBuilder<>* builder) override;
    };

    class AstFpLiteral : public AbstractAstLiteralBase<long double>
    {
    public :
        explicit AstFpLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            const long double value,
            const short bit_count
        ) :
            AbstractAstLiteralBase(
                source,
                source_offset,
                scope,
                LiteralType::FLOAT,
                value,
                bit_count
            ) {}

        std::string to_string() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                             llvm::LLVMContext& context,
                             llvm::IRBuilder<>* builder) override;
    };

    class AstBooleanLiteral : public AbstractAstLiteralBase<bool>
    {
    public:
        explicit AstBooleanLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            const bool value
        ) :
            AbstractAstLiteralBase(
                source,
                source_offset,
                scope,
                LiteralType::BOOLEAN,
                value,
                1 /* Single bit only*/
            ) {}

        std::string to_string() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                             llvm::LLVMContext& context,
                             llvm::IRBuilder<>* builder) override;
    };

    class AstCharLiteral : public AbstractAstLiteralBase<char>
    {
    public:
        explicit AstCharLiteral(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<SymbolRegistry>& scope,
            const char value
        ) :
            AbstractAstLiteralBase(
                source,
                source_offset,
                scope,
                LiteralType::CHAR,
                value,
                8
            ) {}

        std::string to_string() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                             llvm::LLVMContext& context,
                             llvm::IRBuilder<>* builder) override;
    };

    std::optional<std::unique_ptr<AstLiteral>> parse_literal_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& tokens
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_boolean_literal_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_float_literal_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_integer_literal_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_string_literal_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set
    );

    std::optional<std::unique_ptr<AstLiteral>> parse_char_literal_optional(
        const std::shared_ptr<SymbolRegistry>& scope,
        TokenSet& set
    );

    bool is_literal_ast_node(IAstNode* node);
}
