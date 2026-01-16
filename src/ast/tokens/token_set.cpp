//
// Created by Luca Warmenhoven on 12/01/2026.
//

#include "ast/tokens/token_set.h"

using namespace stride::ast;

TokenSet TokenSet::create_subset(const size_t offset, const size_t length) const
{
    const auto start = offset;
    const auto end = offset + length - 1;

    if (start > end || end >= this->size())
    {
        throw std::out_of_range("Invalid range for TokenSet copy");
    }

    std::vector copied_tokens(this->_tokens.begin() + static_cast<std::ptrdiff_t>(offset),
                              this->_tokens.begin() + static_cast<std::ptrdiff_t>(offset +
                                  length));

    return std::move(TokenSet(this->_source, copied_tokens));
}

[[nodiscard]] Token TokenSet::at(const size_t index) const
{
    if (this->size() == 0)
    {
        return END_OF_FILE;
    }

    if (index >= this->size())
    {
        return this->_tokens.back();
    }

    return this->_tokens[index];
}

[[nodiscard]] Token TokenSet::peak(const size_t offset) const
{
    return this->at(this->position() + offset);
}

[[nodiscard]] Token TokenSet::peak_next() const
{
    return this->at(this->_cursor);
}

[[nodiscard]] bool TokenSet::peak_next_eq(const TokenType type) const
{
    return this->peak_next().type == type;
}

void TokenSet::skip(const size_t amount)
{
    this->_cursor += amount;
}

Token TokenSet::expect(const TokenType type)
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

Token TokenSet::expect(TokenType type, const std::string& message)
{
    if (!this->has_next())
    {
        throw parsing_error("No more tokens available");
    }

    if (const auto next = this->peak_next(); next.type != type)
    {
        this->except(ErrorType::SYNTAX_ERROR, message);
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

[[nodiscard]] size_t TokenSet::size() const
{
    return this->_size;
}

[[nodiscard]] size_t TokenSet::position() const
{
    return this->_cursor;
}

[[nodiscard]] size_t TokenSet::remaining() const
{
    return this->size() - this->position();
}

[[nodiscard]] bool TokenSet::has_next() const
{
    return this->remaining() > 0;
}

[[nodiscard]] std::shared_ptr<stride::SourceFile> TokenSet::source() const
{
    return this->_source;
}

[[noreturn]] void TokenSet::except(const Token& token, const ErrorType error_type, const std::string& message) const
{
    throw parsing_error(
        make_source_error(
            *this->source(),
            error_type,
            message,
            token.offset,
            token.lexeme.size()
        )
    );
}


[[noreturn]] void TokenSet::except(const ErrorType error_type, const std::string& message) const
{
    this->except(this->at(this->position()), error_type, message);
}

[[noreturn]] void TokenSet::except(const std::string& message) const
{
    this->except(ErrorType::SYNTAX_ERROR, message);
}
