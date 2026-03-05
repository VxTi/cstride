#include "ast/ast.h"

#include "files.h"
#include "ast/parser.h"
#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
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
