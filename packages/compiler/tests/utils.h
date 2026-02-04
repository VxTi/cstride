#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "files.h"
#include "ast/parser.h"
#include "ast/tokens/tokenizer.h"
#include "ast/parsing_context.h"
#include "stl/stl_functions.h"

namespace stride::tests {

    inline std::unique_ptr<ast::AstBlock> parse_code(const std::string& code) {
        auto source = std::make_shared<SourceFile>("test.sr", code);
        auto tokens = ast::tokenizer::tokenize(source);
        auto context = std::make_shared<ast::ParsingContext>();

        // Predefine symbols like i32, void, etc.
        stl::predefine_symbols(context);

        return ast::parse_sequential(context, tokens);
    }

    inline void assert_parses(const std::string& code) {
        EXPECT_NO_THROW({
            auto block = parse_code(code);
            EXPECT_NE(block, nullptr) << "Parsing returned null for code: " << code;
        });
    }

}
