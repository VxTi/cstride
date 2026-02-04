#include "ast/parser.h"
#include "files.h"
#include "program.h"
#include "ast/modifiers.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/if_statement.h"
#include "ast/nodes/import.h"
#include "ast/nodes/while_loop.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/module.h"
#include "ast/nodes/package.h"
#include "ast/nodes/return.h"
#include "ast/nodes/struct.h"
#include "ast/tokens/tokenizer.h"

using namespace stride::ast;

std::unique_ptr<AstBlock> parser::parse_file(const Program& program, const std::string& source_path)
{
    const auto source_file = read_file(source_path);
    auto tokens = tokenizer::tokenize(source_file);

    return std::move(parse_sequential(program.get_global_scope(), tokens));
}

std::unique_ptr<IAstNode> stride::ast::parse_next_statement(const std::shared_ptr<SymbolRegistry>& registry, TokenSet& set)
{
    // Phase 1 - These sequences are simple to parse; they have no visibility modifiers, hence we can just
    // assume that their first keyword determines their body.
    const auto next_token_type = set.peek_next().get_type();
    auto modifier = VisibilityModifier::PRIVATE;
    int offset = 0;
    switch (next_token_type)
    {
    case TokenType::KEYWORD_IF:
        return parse_if_statement(registry, set);
    case TokenType::KEYWORD_RETURN:
        return parse_return_statement(registry, set);
    case TokenType::KEYWORD_MODULE:
        return parse_module_statement(registry, set);
    case TokenType::KEYWORD_PACKAGE:
        return parse_package_declaration(registry, set);
    case TokenType::KEYWORD_IMPORT:
        return parse_import_statement(registry, set);

        // Modifiers. These are used in the next phase of parsing.
    case TokenType::KEYWORD_PUBLIC:
        modifier = VisibilityModifier::PUBLIC;
        offset = 1;
        break;
    case TokenType::KEYWORD_PRIVATE:
        modifier = VisibilityModifier::PRIVATE;
        offset = 1;
        break;
    default:
        break;
    }

    // Phase 2 -
    switch (set.peek(offset).get_type())
    {
    case TokenType::KEYWORD_ASYNC:
    case TokenType::KEYWORD_FN:
    case TokenType::KEYWORD_EXTERN:
        return parse_fn_declaration(registry, set, modifier);
    case TokenType::KEYWORD_STRUCT:
        return parse_struct_declaration(registry, set, modifier);
    case TokenType::KEYWORD_ENUM:
        return parse_enumerable_declaration(registry, set, modifier);
    case TokenType::KEYWORD_FOR:
        return parse_for_loop_statement(registry, set, modifier);
    case TokenType::KEYWORD_WHILE:
        return parse_while_loop_statement(registry, set, modifier);
    default: break;
    }

    return parse_standalone_expression(registry, set);
}

std::unique_ptr<AstBlock> stride::ast::parse_sequential(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    std::vector<std::unique_ptr<IAstNode>> nodes = {};

    const auto initial_token = set.peek_next();

    while (set.has_next())
    {
        if (auto result = parse_next_statement(registry, set))
        {
            nodes.push_back(std::move(result));
        }
    }

    return std::make_unique<AstBlock>(
        set.get_source(),
        initial_token.get_source_position(),
        registry,
        std::move(nodes)
    );
}
