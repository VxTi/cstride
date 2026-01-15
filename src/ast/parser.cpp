#include "files.h"
#include "ast/parser.h"
#include "ast/nodes/import.h"
#include "ast/nodes/root_node.h"
#include "ast/tokens/tokenizer.h"

using namespace stride::ast;

std::unique_ptr<AstNode> parser::parse(const std::string& source_path)
{
    const auto source_file = read_file(source_path);
    const auto tokens = tokenizer::tokenize(source_file);
    const auto scope_global = Scope(ScopeType::GLOBAL);

    return AstBlockNode::try_parse(scope_global, tokens);
}
