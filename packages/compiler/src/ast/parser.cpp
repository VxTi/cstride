#include "ast/parser.h"
#include "files.h"
#include "program.h"
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

std::unique_ptr<IAstNode> stride::ast::parse_next_statement(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set)
{
    if (is_package_declaration(set))
    {
        return parse_package_declaration(scope, set);
    }
    if (is_module_statement(set))
    {
        return parse_module_statement(scope, set);
    }

    if (is_import_statement(set))
    {
        return parse_import_statement(scope, set);
    }

    if (is_fn_declaration(set))
    {
        return parse_fn_declaration(scope, set);
    }

    if (is_struct_declaration(set))
    {
        return parse_struct_declaration(scope, set);
    }

    if (is_enumerable_declaration(set))
    {
        return parse_enumerable_declaration(scope, set);
    }

    if (is_return_statement(set))
    {
        return parse_return_statement(scope, set);
    }

    if (is_for_loop_statement(set))
    {
        return parse_for_loop_statement(scope, set);
    }

    if (is_while_loop_statement(set))
    {
        return parse_while_loop_statement(scope, set);
    }

    if (is_if_statement(set))
    {
        return parse_if_statement(scope, set);
    }

    return parse_standalone_expression(scope, set);
}

std::unique_ptr<AstBlock> stride::ast::parse_sequential(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    std::vector<std::unique_ptr<IAstNode>> nodes = {};

    const auto initial_token = set.peak_next();

    while (set.has_next())
    {
        if (should_skip_token(set.peak_next().type))
        {
            set.next();
            continue;
        }

        if (auto result = parse_next_statement(scope, set))
        {
            nodes.push_back(std::move(result));
        }
    }

    return std::make_unique<AstBlock>(
        set.source(),
        initial_token.offset,
        scope,
        std::move(nodes)
    );
}
