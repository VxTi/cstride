#pragma once

#include "symbol_table.h"
#include "ast/nodes/blocks.h"

#include <map>
#include <memory>
#include <vector>

namespace stride
{
    struct SourceFile;
}

namespace stride::ast
{
    class TokenSet;
}

namespace stride::ast
{
    using FilePath = std::string;

    // A branch, representing a file in the AST
    class AstBranch
    {
        std::unique_ptr<SourceFile> _source_file;
        std::unique_ptr<AstBlock> _node;

    public:
        explicit AstBranch(
            std::unique_ptr<SourceFile> source_file,
            std::unique_ptr<AstBlock> node
        ) :
            _source_file(std::move(source_file)),
            _node(std::move(node)) {}

        [[nodiscard]]
        const std::string& get_file_content() const
        {
            return this->_source_file->source;
        }

        [[nodiscard]]
        const std::string& get_file_path() const
        {
            return this->_source_file->path;
        }

        [[nodiscard]]
        SourceFile* get_source_file() const
        {
            return this->_source_file.get();
        }

        [[nodiscard]]
        AstBlock* get_node() const
        {
            return this->_node.get();
        }
    };

    class Ast
    {
        std::map<std::string, std::unique_ptr<AstBranch>> _branches;

        static std::pair<FilePath, std::unique_ptr<AstBranch>> parse_file(const FilePath& path);

    public:
        static std::unique_ptr<Ast> parse_files(
            const std::vector<FilePath>& files
        );

        void optimize(); // TODO: Implement

        void print() const;

        const std::map<std::string, std::unique_ptr<AstBranch>>& get_branches()
        {
            return this->_branches;
        }
    };

    std::unique_ptr<IAstNode> parse_next_statement(TokenSet& set);

    std::unique_ptr<AstBlock> parse_sequential(TokenSet& set);
}
