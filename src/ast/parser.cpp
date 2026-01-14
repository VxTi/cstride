#include "ast/parser.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>

#include "ast/nodes/import_node.h"
#include "ast/nodes/root_node.h"
#include "ast/tokens/tokenizer.h"

using namespace stride::ast;

parser::Parser::Parser(const std::string& source_path) : position(0), source_path(source_path)
{
    const std::ifstream file(source_path);

    if (!file)
    {
        throw parsing_error("Failed to open file: " + source_path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    this->tokens = std::move(tokenizer::tokenize(source));
}

std::unique_ptr<AstNode> parser::Parser::parse() const
{
    const auto global_scope = Scope(ScopeType::GLOBAL);

    return AstBlockNode::try_parse(global_scope, tokens);
}
