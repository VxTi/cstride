#pragma once

#include <string>
#include "token.h"
#include "token_set.h"

namespace stride::ast::tokenizer
{
    static bool isWhitespace(const char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    std::unique_ptr<TokenSet> tokenize(const std::string& source);
}
