#include "ast/parser.h"

#include "files.h"
#include "program.h"
#include "ast/modifiers.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/if_statement.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/package.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/struct_declaration.h"
#include "ast/nodes/while_loop.h"
#include "ast/tokens/tokenizer.h"

using namespace stride::ast;

std::unique_ptr<AstBlock> parser::parse_file(
    const Program& program,
    const std::string& source_path
)
{
    const auto source_file = read_file(source_path);
    auto tokens = tokenizer::tokenize(source_file);

    return std::move(parse_sequential(program.get_global_context(), tokens));
}

std::unique_ptr<IAstNode> stride::ast::parse_next_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    // Phase 1 - These sequences are simple to parse; they have no visibility modifiers, hence we
    // can just assume that their first keyword determines their body.
    auto visibility_modifier = VisibilityModifier::NONE;

    switch (set.peek_next().get_type())
    {
    case TokenType::KEYWORD_IF:
        return parse_if_statement(context, set);
    case TokenType::KEYWORD_RETURN:
        return parse_return_statement(context, set);
    case TokenType::KEYWORD_MODULE:
        return parse_module_statement(context, set);
    case TokenType::KEYWORD_PACKAGE:
        return parse_package_declaration(context, set);
    case TokenType::KEYWORD_IMPORT:
        return parse_import_statement(context, set);

    // Modifiers. These are used in the next phase of parsing.
    case TokenType::KEYWORD_PUBLIC:
        visibility_modifier = VisibilityModifier::GLOBAL;
        set.skip(1);
        break;
    case TokenType::KEYWORD_PRIVATE:
        visibility_modifier = VisibilityModifier::NONE;
        set.skip(1);
        break;
    default:
        break;
    }

    // Phase 2 - These sequences may have visibility modifiers, so we need to
    // offset our peek accordingly.
    switch (set.peek_next_type())
    {
    case TokenType::KEYWORD_ASYNC:
    case TokenType::KEYWORD_FN:
    case TokenType::KEYWORD_EXTERN:
        return parse_fn_declaration(context, set, visibility_modifier);
    case TokenType::KEYWORD_STRUCT:
        return parse_struct_declaration(context, set, visibility_modifier);
    case TokenType::KEYWORD_ENUM:
        return parse_enumerable_declaration(context, set, visibility_modifier);
    case TokenType::KEYWORD_FOR:
        return parse_for_loop_statement(context, set, visibility_modifier);
    case TokenType::KEYWORD_WHILE:
        return parse_while_loop_statement(context, set, visibility_modifier);
    case TokenType::KEYWORD_LET:
    case TokenType::KEYWORD_CONST:
        return parse_variable_declaration(context, set, visibility_modifier);
    default:
        break;
    }

    return parse_standalone_expression(context, set);
}

std::unique_ptr<AstBlock> stride::ast::parse_sequential(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    std::vector<std::unique_ptr<IAstNode>> nodes = {};

    const auto initial_token = set.peek_next();

    while (set.has_next())
    {
        if (auto result = parse_next_statement(context, set))
        {
            nodes.push_back(std::move(result));
        }
    }

    return std::make_unique<AstBlock>(
        initial_token.get_source_fragment(),
        context,
        std::move(nodes));
}
