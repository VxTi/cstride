#include "../../../include/ast/tokens/tokenizer.h"

#include "errors.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::unique_ptr<TokenSet> tokenizer::tokenize(const std::shared_ptr<SourceFile>& source_file)
{
    auto tokens = std::vector<Token>();

    const auto src = source_file->source;

    for (std::size_t i = 0; i < src.length();) // Note: `i` is incremented manually
    {
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
                std::string value = match.str(0);
                tokens.emplace_back(tokenDefinition.type, i, value);

                i += value.length();
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            throw parsing_error(
                make_source_error(
                    *source_file,
                    ErrorType::SYNTAX_ERROR,
                    "Unexpected character encountered",
                    i, 1
                )
            );
        }
    }

    return std::make_unique<TokenSet>(source_file, tokens);
}
