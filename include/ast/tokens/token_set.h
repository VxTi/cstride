//
// Created by Luca Warmenhoven on 12/01/2026.
//

#pragma once

#include <algorithm>
#include <vector>

#include "errors.h"
#include "ast/tokens/token.h"

namespace stride::ast
{
    class TokenSet
    {
        const SourceFile& _source;

        size_t _cursor;
        size_t _size;

        std::vector<Token> _tokens;

    public:
        explicit TokenSet(const SourceFile& source, std::vector<Token>& tokens) :
            _source(source),
            _cursor(0),
            _size(tokens.size()),
            _tokens(std::move(tokens))
        {
        }

        [[nodiscard]] std::unique_ptr<TokenSet> create_subset(size_t offset, size_t length) const;

        [[nodiscard]] Token at(size_t index) const;

        [[nodiscard]] Token peak(size_t offset) const;

        [[nodiscard]] Token peak_next() const;

        [[nodiscard]] bool peak_next_eq(TokenType type) const;

        void skip(size_t amount);

        Token expect(TokenType type);

        Token next();

        [[nodiscard]] size_t size() const;

        [[nodiscard]] size_t position() const;

        [[nodiscard]] size_t remaining() const;

        [[nodiscard]] bool has_next() const;

        [[nodiscard]] SourceFile source() const;

        [[noreturn]] void except(const Token& token, ErrorType error_type, const std::string& message) const;

        [[noreturn]] void except(ErrorType error_type, const std::string& message) const;

        [[noreturn]] void except(const std::string& message) const;
    };
}
