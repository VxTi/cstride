//
// Created by Luca Warmenhoven on 12/01/2026.
//

#include "ast/tokens/token_set.h"

using namespace stride::ast;

TokenSet TokenSet::create_subset(const int64_t offset, const int64_t length) const
{
    const auto start = offset;

    if (const auto end = offset + length - 1; start > end || end >= this->size())
    {
        throw std::out_of_range("Invalid range for TokenSet copy");
    }

    std::vector copied_tokens(
        this->_tokens.begin() + offset,
        this->_tokens.begin() + offset + length
    );

    return std::move(TokenSet(this->_source, copied_tokens));
}

Token TokenSet::last() const
{
    if (this->size() == 0)
    {
        return END_OF_FILE;
    }
    return this->_tokens.back();
}

Token TokenSet::at(const int64_t index) const
{
    if (this->size() == 0 || this->remaining() == 0 || index < 0)
    {
        return END_OF_FILE;
    }

    if (index >= this->size())
    {
        return this->_tokens.back();
    }

    return this->_tokens[index];
}

Token TokenSet::peek(const int64_t offset) const
{
    return this->at(this->position() + offset);
}

TokenType TokenSet::peek_next_type() const
{
    return this->peek_next().get_type();
}

Token TokenSet::peek_next() const
{
    return this->at(this->position());
}


bool TokenSet::peek_next_eq(const TokenType type) const
{
    return this->peek_eq(type, 0);
}

bool TokenSet::peek_eq(const TokenType type, const int64_t offset) const
{
    return this->peek(offset).get_type() == type;
}

void TokenSet::skip(const int64_t amount)
{
    this->_cursor += amount;
}

Token TokenSet::expect(const TokenType type)
{
    if (!this->has_next())
    {
        const auto last_token_pos = this->last().get_source_position();

        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            std::format(
                "Expected '{}', but reached end of block",
                token_type_to_str(type)
            ),
            *this->get_source(),
            SourcePosition(last_token_pos.offset + last_token_pos.length - 1, 1)
        );
    }

    if (const auto next_type = this->peek_next().get_type(); next_type != type)
    {
        this->throw_error(
            ErrorType::SYNTAX_ERROR,
            std::format(
                "Expected '{}' but found '{}'",
                token_type_to_str(type),
                token_type_to_str(next_type)
            )
        );
    }

    return this->next();
}

Token TokenSet::expect(const TokenType type, const std::string& message)
{
    if (!this->has_next())
    {
        const auto last_token_pos = this->last().get_source_position();

        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            std::format(
                "Expected '{}', but reached end of block",
                token_type_to_str(type)
            ),
            *this->get_source(),
            SourcePosition(last_token_pos.offset + last_token_pos.length - 1, 1)
        );
    }

    if (this->peek_next().get_type() != type)
    {
        this->throw_error(ErrorType::SYNTAX_ERROR, message);
    }

    return this->next();
}

Token TokenSet::next()
{
    if (this->remaining() == 0)
    {
        return END_OF_FILE;
    }
    return this->_tokens[this->_cursor++];
}

int64_t TokenSet::size() const
{
    return this->_size;
}

int64_t TokenSet::position() const
{
    return this->_cursor;
}

int64_t TokenSet::remaining() const
{
    return this->size() - this->position();
}

bool TokenSet::has_next() const
{
    return this->remaining() > 0
        && !this->peek_next_eq(TokenType::END_OF_FILE);
}

std::shared_ptr<stride::SourceFile> TokenSet::get_source() const
{
    return this->_source;
}

[[noreturn]] void TokenSet::throw_error(
    const Token& token,
    const ErrorType error_type,
    const std::string& message
) const
{
    throw parsing_error(
        error_type,
        message,
        *this->get_source(),
        token.get_source_position()
    );
}

[[noreturn]] void TokenSet::throw_error(const ErrorType error_type, const std::string& message) const
{
    this->throw_error(this->at(this->position()), error_type, message);
}

[[noreturn]] void TokenSet::throw_error(const std::string& message) const
{
    this->throw_error(ErrorType::SYNTAX_ERROR, message);
}
