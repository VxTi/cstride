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
        std::string _source;

        size_t _cursor;
        size_t _size;

        std::vector<Token> _tokens;

    public:
        explicit TokenSet(std::string source, std::vector<Token>& tokens) :
            _source(std::move(source)),
            _cursor(0),
            _size(tokens.size()),
            _tokens(std::move(tokens))
        {
        }

        [[nodiscard]] std::unique_ptr<TokenSet> copy(const size_t offset, const size_t length) const
        {
            std::vector subset_tokens(
                this->_tokens.begin() + offset,
                this->_tokens.begin() + offset + length
            );
            return std::make_unique<TokenSet>(this->_source, subset_tokens);
        }

        [[nodiscard]] Token at(const size_t index) const
        {
            if (index >= this->size())
            {
                return END_OF_FILE;
            }

            return this->_tokens[index];
        }

        [[nodiscard]] Token peak(const size_t offset) const
        {
            return this->at(this->position() + offset);
        }

        [[nodiscard]] Token peak_next() const
        {
            return this->at(this->_cursor);
        }

        [[nodiscard]] bool peak_next_eq(const TokenType type) const
        {
            return this->peak_next().type == type;
        }

        /**
         * Optionally skip the provided token and peak forward
         */
        bool skip_optional(const TokenType type)
        {
            if (this->peak_next().type == type)
            {
                this->next();
                return true;
            }
            return false;
        }

        void skip(const size_t amount)
        {
            this->_cursor += amount;
        }

        Token expect(const TokenType type)
        {
            if (!this->has_next())
            {
                throw parsing_error("No more tokens available");
            }

            if (const auto next = this->peak_next(); next.type != type)
            {
                this->except(
                    ErrorType::SYNTAX_ERROR,
                    std::format(
                        "Expected '{}' but found '{}'",
                        token_type_to_str(type),
                        token_type_to_str(next.type)
                    )
                );
            }

            return this->next();
        }

        Token next()
        {
            if (this->remaining() == 0)
            {
                return END_OF_FILE;
            }
            return this->_tokens[this->_cursor++];
        }

        [[nodiscard]] size_t size() const
        {
            return this->_size;
        }

        [[nodiscard]] size_t position() const
        {
            return this->_cursor;
        }

        [[nodiscard]] size_t remaining() const
        {
            return this->size() - this->position();
        }

        [[nodiscard]] bool has_next() const
        {
            return this->remaining() > 0;
        }

        [[nodiscard]] const std::string& source() const
        {
            return this->_source;
        }

        [[noreturn]] void except(const Token& token, const ErrorType error_type, const std::string& message) const
        {
            throw parsing_error(
                make_source_error(
                    this->source(),
                    error_type,
                    message,
                    token.offset,
                    token.lexeme.size()
                )
            );
        }


        [[noreturn]] void except(const ErrorType error_type, const std::string& message) const
        {
            this->except(this->at(this->position()), error_type, message);
        }

        [[noreturn]] void except(const std::string& message) const
        {
            this->except(ErrorType::SYNTAX_ERROR, message);
        }
    };
}
