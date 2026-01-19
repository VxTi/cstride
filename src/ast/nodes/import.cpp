#include <sstream>

#include "ast/nodes/import.h"
#include "errors.h"

using namespace stride::ast;

bool stride::ast::is_import_statement(const TokenSet& tokens)
{
    return tokens.peak_next().type == TokenType::KEYWORD_USE;
}

/**
 * Will consume the import module parts from the token set.
 * This will convert the following example into the below given
 * symbol:
 * <code>
 * use sys::io::{File, Console};
 * </code>
 * Here, <code>sys::io</code> will be converted into the symbol <code>sys__io</code>
 */
std::string consume_import_module_base(TokenSet& tokens)
{
    const auto base = tokens.expect(TokenType::IDENTIFIER);
    std::vector<std::string> parts = {base.lexeme};

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
        imploded << SEGMENT_DELIMITER << parts[i];
    }

    return imploded.str();
}

/**
 * Attempts to parse a list of symbols from the given TokenSet.
 *
 * This function expects a double colon (::) followed by one or more identifiers.
 * It parses the identifiers and returns them as a vector of Symbol objects.
 */
std::vector<std::string> consume_import_submodules(TokenSet& tokens)
{
    tokens.expect(TokenType::DOUBLE_COLON);
    tokens.expect(TokenType::LBRACE);
    const auto first = tokens.expect(TokenType::IDENTIFIER, "Expected symbol in import list");

    std::vector<std::string> submodules = {first.lexeme};

    while (tokens.peak(0) == TokenType::COMMA && tokens.peak(1) == TokenType::IDENTIFIER)
    {
        tokens.expect(TokenType::COMMA);
        const auto submodule_iden = tokens.expect(TokenType::IDENTIFIER, "Expected symbol in import list");

        submodules.push_back(submodule_iden.lexeme);
    }

    tokens.expect(TokenType::RBRACE);

    if (submodules.empty())
    {
        tokens.throw_error(
            "Expected at least one symbol in import submodule list"
        );
    }

    return submodules;
}

/**
 * Attempts to parse an import expression from the given TokenSet.
 */
std::unique_ptr<AstImport> stride::ast::parse_import_statement(const Scope& scope, TokenSet& set)
{
    if (scope.type != ScopeType::GLOBAL)
    {
        set.throw_error(
            std::format(
                "Import statements are only allowed in global scope, but was found in {} scope",
                scope_type_to_str(scope.type)
            )
        );
    }
    const auto reference_token = set.expect(TokenType::KEYWORD_USE);

    const auto import_module = consume_import_module_base(set);
    const auto import_list = consume_import_submodules(set);

    set.expect(TokenType::SEMICOLON);

    const Dependency dependency = {
        .module_base = import_module,
        .submodules = import_list
    };


    return std::make_unique<AstImport>(
        set.source(),
        reference_token.offset,
        dependency
    );
}

std::string AstImport::to_string()
{
    std::ostringstream deps_stream;
    for (size_t i = 0; i < this->get_submodules().size(); ++i)
    {
        deps_stream << this->get_submodules()[i];
        if (i < this->get_submodules().size() - 1)
        {
            deps_stream << ", ";
        }
    }
    return std::format("Import [{}] {{ {} }}", this->get_module(), deps_stream.str());
};
