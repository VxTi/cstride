#pragma once
#include <memory>

namespace stride
{
    class Program;
}

namespace stride::ast
{
    class ParsingContext;
    class IAstNode;
    class TokenSet;
    class AstBlock;

    namespace parser
    {
        std::unique_ptr<AstBlock> parse_file(const Program& program, const std::string& path);
    }

    std::unique_ptr<IAstNode> parse_next_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    std::unique_ptr<AstBlock> parse_sequential(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );
} // namespace stride::ast
