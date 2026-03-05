#pragma once
#include "parsing_context.h"

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

    class Ast
    {
        std::map<FilePath, std::unique_ptr<AstBlock>> _files{};
        ParsingContext _global_context;

        static std::pair<FilePath, std::unique_ptr<AstBlock>> parse_file(const FilePath& path);

    public:
        static std::unique_ptr<Ast> parse_files(
            const std::vector<FilePath>& files
        );

        void optimize(); // TODO: Implement

        void print() const;

        const std::map<FilePath, std::unique_ptr<AstBlock>>& get_files()
        {
            return this->_files;
        }
    };
}
