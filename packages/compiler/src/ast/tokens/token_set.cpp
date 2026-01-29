//
// Created by Luca Warmenhoven on 12/01/2026.
//

#include "ast/tokens/token_set.h"

using namespace stride::ast;

TokenSet TokenSet::create_subset(const size_t offset, const size_t length) const
{
    const auto start = offset;

    if (const auto end = offset + length - 1; start > end || end >= this->size())
    {
        throw std::out_of_range("Invalid range for TokenSet copy");
    }

    std::vector copied_tokens(
        this->_tokens.begin() + static_cast<std::ptrdiff_t>(offset),
        this->_tokens.begin() + static_cast<std::ptrdiff_t>(offset +
            length)
    );

    return std::move(TokenSet(this->_source, copied_tokens));
}

Token TokenSet::at(const size_t index) const
{
    if (this->size() == 0 || this->remaining() == 0)
    {
        return END_OF_FILE;
    }

    if (index >= this->size())
    {
        return this->_tokens.back();
    }

    return this->_tokens[index];
}

Token TokenSet::peak(const size_t offset) const
{
    return this->at(this->position() + offset);
}

TokenType TokenSet::peak_next_type() const
{
    return this->peak_next().get_type();
}

Token TokenSet::peak_next() const
{
    return this->at(this->_cursor);
}


bool TokenSet::peak_next_eq(const TokenType type) const
{
    return this->peak_eq(type, 0);
}

bool TokenSet::peak_eq(const TokenType type, const size_t offset) const
{
    return this->peak(offset).get_type() == type;
}

void TokenSet::skip(const size_t amount)
{
    this->_cursor += amount;
}

Token TokenSet::expect(const TokenType type)
{
    if (!this->has_next())
    {
        this->throw_error(
            ErrorType::SYNTAX_ERROR,
            std::format(
                "Expected '{}', but reached end of block",
                token_type_to_str(type)
            )
        );
    }

    if (const auto next_type = this->peak_next().get_type(); next_type != type)
    {
        if (should_skip_token(next_type))
        {
            this->next();
            return this->expect(type);
        }
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
        this->throw_error(
            ErrorType::SYNTAX_ERROR,
            std::format(
                "Expected '{}', but reached end of block",
                token_type_to_str(type)
            )
        );
    }

    if (const auto next_type = this->peak_next().get_type(); next_type != type)
    {
        if (should_skip_token(next_type))
        {
            this->next();
            return this->expect(type);
        }
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

size_t TokenSet::size() const
{
    return this->_size;
}

size_t TokenSet::position() const
{
    return this->_cursor;
}

size_t TokenSet::remaining() const
{
    return this->size() - this->position();
}

bool TokenSet::has_next() const
{
    return this->remaining() > 0
        && !this->peak_next_eq(TokenType::END_OF_FILE);
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
        make_source_error(
            error_type,
            message,
            *this->get_source(),
            token.get_source_position()
        )
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


bool stride::ast::should_skip_token(const TokenType type)
{
    switch (type)
    {
    case TokenType::COMMENT:
    case TokenType::COMMENT_MULTILINE:
    case TokenType::END_OF_FILE: return true;
    default: return false;
    }
}
