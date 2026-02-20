#include "../../../include/ast/tokens/tokenizer.h"

#include <sstream>

#include "errors.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

bool should_ignore_token_type(TokenType type)
{
    switch (type)
    {
    case TokenType::COMMENT:
    case TokenType::COMMENT_MULTILINE: return true;
    default: return false;
    }
}

TokenSet tokenizer::tokenize(const std::shared_ptr<SourceFile>& source_file)
{
    auto tokens = std::vector<Token>();

    const auto src = source_file->source;

    bool is_string = false;
    size_t string_start = 0;
    std::ostringstream string_stream;

    for (std::size_t i = 0; i < src.length();) // Note: `i` is incremented manually
    {
        /**
         * String parsing - This is done a little differently, since strings can be ambiguous
         */

        if (is_string)
        {
            // Check for closing quote, ensuring it's not escaped.
            // We need to count preceding backslashes to handle cases like "\\" correctly.
            if (src[i] == '"')
            {
                size_t backslash_count = 0;
                size_t check_index = i;
                while (check_index > 0 && src[check_index - 1] == '\\')
                {
                    backslash_count++;
                    check_index--;
                }

                // If there's an odd number of backslashes, the quote is escaped.
                if (backslash_count % 2 == 0)
                {
                    size_t length = i - string_start;
                    std::string raw_val = src.substr(string_start, length);
                    std::string val = escape_string(raw_val);

                    tokens.push_back(
                        Token(
                            TokenType::STRING_LITERAL,
                            SourceLocation(source_file, string_start - 1, length + 2),
                            val
                        )
                    );

                    is_string = false;
                }
            }

            i++;
            continue;
        }


        if (src[i] == '\"')
        {
            // Start of string
            is_string = true;
            string_start = i + 1; // Start after the quote
            i++;
            continue;
        }

        if (isWhitespace(src[i]))
        {
            i++;
            continue;
        }

        bool matched = false;
        for (const auto& tokenDefinition : tokenTypes)
        {
            // Define the search range starting from the current index i
            const auto searchStart = src.cbegin() + i;
            const auto searchEnd = src.cend();

            // Use match_continuous to ensure the regex matches exactly at searchStart
            if (std::smatch match; std::regex_search(
                    searchStart, searchEnd, match,
                    tokenDefinition.pattern,
                    std::regex_constants::match_continuous
                )
            )
            {
                std::string lexeme = match.str(0);

                if (!should_ignore_token_type(tokenDefinition.type))
                {
                    tokens.emplace_back(
                        tokenDefinition.type,
                        SourceLocation(source_file, i, lexeme.length()),
                        lexeme
                    );
                }

                i += lexeme.length();
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            throw parsing_error(
                ErrorType::SYNTAX_ERROR,
                "Unexpected character encountered",
                SourceLocation(source_file, i, 1)
            );
        }
    }

    return TokenSet(source_file, tokens);
}

// This allows one to type `\0` in a string and have it actually
// result in a null character, instead of two separate characters. (`\` and `0`)
std::string tokenizer::escape_string(const std::string& raw_string)
{
    std::string result;
    result.reserve(raw_string.size());

    for (size_t i = 0; i < raw_string.length(); ++i)
    {
        if (raw_string[i] == '\\' && i + 1 < raw_string.length())
        {
            switch (raw_string[i + 1])
            {
            case 'n': result += '\n';
                break;
            case 't': result += '\t';
                break;
            case 'r': result += '\r';
                break;
            case '\\': result += '\\';
                break;
            case '"': result += '"';
                break;
            case '0': result += '\0';
                break;
            case 'a': result += '\a';
                break;
            case 'b': result += '\b';
                break;
            case 'f': result += '\f';
                break;
            case 'v': result += '\v';
                break;
            case 'x':
                if (i + 3 < raw_string.length())
                {
                    std::string hex_str = raw_string.substr(i + 2, 2);
                    char hex_char = static_cast<char>(std::stoi(hex_str, nullptr, 16));
                    result += hex_char;
                    i += 2; // Skip the two hex digits
                }
                else
                {
                    // Invalid hex escape, keep as-is
                    result += raw_string[i];
                    result += raw_string[i + 1];
                }
                break;
            default:
                // Unknown escape sequence, keep as-is
                result += raw_string[i];
                result += raw_string[i + 1];
                break;
            }
            ++i; // Skip the next character
        }
        else
        {
            result += raw_string[i];
        }
    }

    return result;
}
