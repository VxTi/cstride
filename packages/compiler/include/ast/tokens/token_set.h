#pragma once

#include "ast/tokens/token.h"
#include "errors.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace stride::ast
{
    class TokenSet
    {
        std::shared_ptr<SourceFile> _source;

        int64_t _cursor;
        int64_t _size;

        std::vector<Token> _tokens;

    public:
        explicit TokenSet(std::shared_ptr<SourceFile> source,
                          std::vector<Token>& tokens) :
            _source(std::move(source)),
            _cursor(0),
            _size(tokens.size()),
            _tokens(std::move(tokens)) {}

        [[nodiscard]]
        TokenSet create_subset(int64_t offset, int64_t length) const;

        [[nodiscard]]
        Token last() const;

        [[nodiscard]]
        Token at(int64_t index) const;

        [[nodiscard]]
        Token peek(int64_t offset) const;

        [[nodiscard]]
        Token peek_next() const;

        [[nodiscard]]
        TokenType peek_next_type() const;

        [[nodiscard]]
        bool peek_next_eq(TokenType type) const;

        [[nodiscard]]
        bool peek_eq(TokenType type, int64_t offset) const;

        void skip(int64_t amount);

        Token expect(TokenType type);

        Token expect(TokenType type, const std::string& message);

        Token next();

        [[nodiscard]]
        int64_t size() const;

        [[nodiscard]]
        int64_t position() const;

        [[nodiscard]]
        int64_t remaining() const;

        [[nodiscard]]
        bool has_next() const;

        [[nodiscard]]
        std::shared_ptr<SourceFile> get_source() const;

        [[noreturn]]
        void throw_error(const Token& token,
                         ErrorType error_type,
                         const std::string& message)
        const;

        [[noreturn]]
        void throw_error(ErrorType error_type,
                         const std::string& message) const;

        [[noreturn]]
        void throw_error(const std::string& message) const;
    };
} // namespace stride::ast
