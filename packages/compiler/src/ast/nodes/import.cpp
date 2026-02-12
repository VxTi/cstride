#include <sstream>

#include "ast/nodes/import.h"
#include "errors.h"
#include "formatting.h"

using namespace stride::ast;

/**
 * Will consume the import module parts from the token set.
 * This will convert the following example into the below given
 * symbol:
 * <code>
 * use sys::io::{File, Console};
 * </code>
 * Here, <code>sys::io</code> will be converted into the symbol <code>sys__io</code>
 */
Symbol consume_import_module_base(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& tokens
)
{
    const auto base = tokens.expect(
        TokenType::IDENTIFIER,
        "Expected package name after 'use' keyword, e.g., 'use <package>::{ ... }'"
    );
    std::vector parts = {base.get_lexeme()};

    while (tokens.peek(0) == TokenType::DOUBLE_COLON)
    {
        tokens.next();
        const auto part = tokens.expect(TokenType::IDENTIFIER, "Expected module name in import statement");

        parts.push_back(part.get_lexeme());
    }

    // TODO: Fix source position. This is currently only the base, which means the rest of the statement
    //  won't be used in error message logging.
    return resolve_internal_name(context->get_name(), base.get_source_position(), parts);
}

/**
 * Attempts to parse a list of symbols from the given TokenSet.
 *
 * This function expects a double colon (::) followed by one or more identifiers.
 * It parses the identifiers and returns them as a vector of Symbol objects.
 */
std::vector<Symbol> consume_import_submodules(TokenSet& tokens)
{
    tokens.expect(TokenType::DOUBLE_COLON, "Expected a '::' before import submodule list");
    tokens.expect(TokenType::LBRACE, "Expected opening brace with modules after '::', e.g., {module1, module2, ...}");
    const auto first = tokens.expect(TokenType::IDENTIFIER, "Expected module name in import list");

    std::vector<Symbol> submodules = {Symbol(first.get_source_position(), first.get_lexeme())};

    while (tokens.peek(0) == TokenType::COMMA && tokens.peek(1) == TokenType::IDENTIFIER)
    {
        tokens.expect(TokenType::COMMA, "Expected comma between module names in import list");
        const auto submodule_iden = tokens.expect(TokenType::IDENTIFIER, "Expected module name in import list");

        submodules.emplace_back(submodule_iden.get_source_position(), submodule_iden.get_lexeme());
    }

    tokens.expect(TokenType::RBRACE, "Expected closing brace after import list");

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
std::unique_ptr<AstImport> stride::ast::parse_import_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    if (context->get_current_scope_type() != ScopeType::GLOBAL)
    {
        set.throw_error(
            std::format(
                "Import statements are only allowed in global scope, but was found in {} scope",
                scope_type_to_str(context->get_current_scope_type())
            )
        );
    }
    const auto reference_token = set.expect(TokenType::KEYWORD_IMPORT);
    // With the guard clause, this will always be the case.

    const auto import_module = consume_import_module_base(context, set);
    const auto import_list = consume_import_submodules(set);

    const Dependency dependency = {
        .module_base = import_module,
        .submodules  = import_list
    };


    return std::make_unique<AstImport>(
        set.get_source(),
        reference_token.get_source_position(),
        context,
        dependency
    );
}

std::string AstImport::to_string()
{
    std::vector<std::string> modules;
    modules.reserve(this->get_submodules().size());

    for (const auto& module : this->get_submodules())
    {
        modules.push_back(module.name);
    }
    return std::format("Import [{}] {{ {} }}", this->get_module().name, join(modules, ", "));
};
