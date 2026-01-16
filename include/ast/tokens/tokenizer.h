#pragma once

#include "token.h"
#include "token_set.h"

namespace stride::ast::tokenizer
{
    static bool isWhitespace(const char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    TokenSet tokenize(const std::shared_ptr<SourceFile>& source_file);

    std::string escape_string(const std::string& raw_string);
}
