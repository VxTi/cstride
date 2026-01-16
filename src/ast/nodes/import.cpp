#include <sstream>

#include "ast/nodes/import.h"
#include "errors.h"

using namespace stride::ast;

bool AstImportNode::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next().type == TokenType::KEYWORD_USE;
}

auto delimiter = "__";

/**
 * Will consume the import module parts from the token set.
 * This will convert the following example into the below given
 * symbol:
 * <code>
 * use sys::io::{File, Console};
 * </code>
 * Here, <code>sys::io</code> will be converted into the symbol <code>sys__io</code>
 */
Symbol consume_import_module_base(TokenSet& tokens)
{
    const auto base = tokens.expect(TokenType::IDENTIFIER);
    std::vector parts = {base.lexeme};

    while (tokens.peak(0) == TokenType::DOUBLE_COLON && tokens.peak(1) == TokenType::IDENTIFIER)
    {
        tokens.expect(TokenType::DOUBLE_COLON);
        const auto part = tokens.expect(TokenType::IDENTIFIER);

        parts.push_back(part.lexeme);
    }

    std::ostringstream imploded;

    imploded << parts[0];
    for (size_t i = 1; i < parts.size(); ++i)
    {
        imploded << delimiter << parts[i];
    }

    return Symbol(imploded.str());
}

/**
 * Attempts to parse a list of symbols from the given TokenSet.
 *
 * This function expects a double colon (::) followed by one or more identifiers.
 * It parses the identifiers and returns them as a vector of Symbol objects.
 */
std::vector<Symbol> consume_import_list(TokenSet& tokens)
{
    tokens.expect(TokenType::DOUBLE_COLON);
    tokens.expect(TokenType::LBRACE);
    const auto first = tokens.expect(TokenType::IDENTIFIER, "Expected symbol in import list");

    std::vector<Symbol> symbols = {Symbol(first.lexeme)};

    while (tokens.peak(0) == TokenType::COMMA && tokens.peak(1) == TokenType::IDENTIFIER)
    {
        tokens.expect(TokenType::COMMA);
        const auto next = tokens.expect(TokenType::IDENTIFIER, "Expected symbol in import list");

        symbols.push_back(Symbol(next.lexeme));
    }

    tokens.expect(TokenType::RBRACE);

    if (symbols.empty())
    {
        tokens.except(
            "Expected at least one symbol in import list"
        );
    }

    return symbols;
}

/**
 * Attempts to parse an import expression from the given TokenSet.
 */
std::unique_ptr<AstImportNode> AstImportNode::try_parse(const Scope& scope, TokenSet& tokens)
{
    if (scope.type != ScopeType::GLOBAL)
    {
        tokens.except(
            std::format(
                "Import statements are only allowed in global scope, but was found in {} scope",
                scope_type_to_str(scope.type)
            )
        );
    }
    tokens.expect(TokenType::KEYWORD_USE);

    const auto import_module = consume_import_module_base(tokens);
    const auto import_list = consume_import_list(tokens);

    tokens.expect(TokenType::SEMICOLON);


    return std::make_unique<AstImportNode>(AstImportNode(import_module, import_list));
}

std::string AstImportNode::to_string()
{
    std::ostringstream deps_stream;
    for (size_t i = 0; i < this->dependencies.size(); ++i)
    {
        deps_stream << dependencies[i].value;
        if (i < dependencies.size() - 1)
        {
            deps_stream << ", ";
        }
    }
    return std::format("Import [{}] {{ {} }}", module.value, deps_stream.str());
};
