#include "files.h"
#include "ast/parser.h"
#include "ast/nodes/import.h"
#include "ast/nodes/blocks.h"
#include "ast/tokens/tokenizer.h"

using namespace stride::ast;

std::unique_ptr<IAstNode> parser::parse(const std::string& source_path)
{
    const auto scope_global = Scope(ScopeType::GLOBAL);

    const auto source_file = read_file(source_path);
    auto tokens = tokenizer::tokenize(source_file);

    return AstBlock::try_parse(scope_global, tokens);
}
