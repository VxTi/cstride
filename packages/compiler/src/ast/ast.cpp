#include "ast/ast.h"

#include "files.h"
#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/control_flow_statements.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/package.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/type_definition.h"
#include "ast/nodes/while_loop.h"
#include "ast/tokens/tokenizer.h"

#include <future>
#include <iostream>

using namespace stride::ast;

std::unique_ptr<Ast> Ast::parse_files(const std::vector<FilePath>& files)
{
    auto ast = std::make_unique<Ast>();

    std::vector<std::future<std::pair<FilePath, std::unique_ptr<AstBlock>>>> futures;
    futures.reserve(files.size());

    for (const auto& file : files)
    {
        futures.push_back(
            std::async(
                std::launch::async,
                [file]
                {
                    return parse_file(file);
                }
            )
        );
    }

    for (auto& future : futures)
    {
        auto [file_path, node] = future.get();

        ast->_files.emplace(file_path, std::move(node));
    }

    return ast;
}

std::pair<FilePath, std::unique_ptr<AstBlock>> Ast::parse_file(const FilePath& path)
{
    const auto source_file = read_file(path);
    auto tokens = tokenizer::tokenize(source_file);

    const auto context = std::make_shared<ParsingContext>();

    auto file_node = parse_sequential(context, tokens);

    return { path, std::move(file_node) };
}

void Ast::print() const
{
    for (const auto& [file_name, node] : this->_files)
    {
        std::cout << "--- " << file_name << " --- " << std::endl;
        std::cout << node->to_string() << std::endl;
    }
}

void Ast::optimize()
{
    for (const auto& [file_name, node] : this->_files)
    {
        std::vector<std::unique_ptr<IAstNode>> new_children;
        for (auto& child_ptr : node->get_children())
        {
            IAstNode* child = child_ptr.get();

            if (auto* reducible = dynamic_cast<IReducible*>(child))
            {
                if (std::unique_ptr<IAstNode> reduced = reducible->reduce().value())
                {
                    // Note: Assuming reduce() returns a new raw pointer or
                    // you manage ownership correctly in your AST implementation.
                    new_children.push_back(std::move(reduced));
                    std::cout << "Optimized node: " << child->to_string() << " to "
                        << reduced->to_string() << std::endl;
                    continue;
                }
            }
            // If not reducible or reduction failed, move the original node
            // This requires changing the loop to take ownership or clone
            new_children.push_back(std::unique_ptr<ast::IAstNode>(child));
        }

        this->_files[file_name] = std::make_unique<AstBlock>(
            node->get_source_fragment(),
            node->get_context(),
            std::move(new_children)
        );
    }
}

std::unique_ptr<IAstNode> stride::ast::parse_next_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    // Phase 1 - These sequences are simple to parse; they have no visibility modifiers, hence we
    // can just assume that their first keyword determines their body.
    auto visibility_modifier = VisibilityModifier::PACKAGE_PUBLIC;

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
    case TokenType::KEYWORD_CONTINUE:
        return parse_continue_statement(context, set);
    case TokenType::KEYWORD_BREAK:
        return parse_break_statement(context, set);

    // Modifiers. These are used in the next phase of parsing.
    case TokenType::KEYWORD_PUBLIC:
        visibility_modifier = VisibilityModifier::PUBLIC;
        set.skip(1);
        break;
    case TokenType::KEYWORD_PRIVATE:
        visibility_modifier = VisibilityModifier::PRIVATE;
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
    case TokenType::KEYWORD_TYPE:
        return parse_type_definition(context, set, visibility_modifier);
    case TokenType::KEYWORD_ENUM:
        return parse_enum_type_definition(context, set, visibility_modifier);
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
    TokenSet& set
)
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
