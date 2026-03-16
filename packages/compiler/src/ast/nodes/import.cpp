#include "ast/nodes/import.h"

#include "errors.h"
#include "formatting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

/**
 * Attempts to parse a list of symbols from the given TokenSet.
 *
 * This function expects a double colon (::) followed by one or more identifiers.
 * It parses the identifiers and returns them as a vector of Symbol objects.
 */
std::vector<std::unique_ptr<AstIdentifier>> consume_import_submodules(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    set.expect(TokenType::DOUBLE_COLON, "Expected a '::' before import submodule list");
    set.expect(TokenType::LBRACE, "Expected opening brace with modules after '::'");

    std::vector<std::unique_ptr<AstIdentifier>> import_list;
    import_list.push_back(parse_segmented_identifier(
        context,
        set,
        "Expected module name after '::' in import list"
    ));

    while (set.peek(0) == TokenType::COMMA && set.peek(1) ==
        TokenType::IDENTIFIER)
    {
        set.expect(TokenType::COMMA, "Expected comma between module names in import list");

        import_list.emplace_back(
            parse_segmented_identifier(
                context,
                set,
                "Expected module name after '::' in import list"
            )
        );
    }

    // Consume optional trailing comma
    if (set.peek_next_eq(TokenType::COMMA))
    {
        set.next();
    }

    set.expect(TokenType::RBRACE, "Expected closing brace or comma after import list");
    set.expect(TokenType::SEMICOLON, "Expected semicolon after import list");

    if (import_list.empty())
    {
        set.throw_error("Expected at least one symbol in import submodule list");
    }

    return std::move(import_list);
}

/**
 * Attempts to parse an import expression from the given TokenSet.
 */
std::unique_ptr<AstImport> stride::ast::parse_import_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_IMPORT);

    auto package_identifier = parse_segmented_identifier(
        context,
        set,
        "Expected module name after '::'"
    );

    auto import_list = consume_import_submodules(context, set);

    return std::make_unique<AstImport>(
        SourceFragment::join(reference_token.get_source_fragment(), package_identifier->get_source_fragment()),
        context,
        std::move(package_identifier),
        std::move(import_list)
    );
}

void AstImport::validate()
{
    if (this->get_context()->get_context_type() != ContextType::GLOBAL)
    {
        throw parsing_error(
            ErrorType::SYNTAX_ERROR,
            std::format(
                "Import statements are only allowed in global scope, but was found in {} scope",
                scope_type_to_str(this->get_context()->get_context_type()
                )
            ),
            this->get_source_fragment()
        );
    }
}

std::unique_ptr<IAstNode> AstImport::clone()
{
    std::vector<std::unique_ptr<AstIdentifier>> cloned_submodules;
    cloned_submodules.reserve(this->_import_list.size());

    for (const auto& submodule : this->_import_list)
    {
        cloned_submodules.push_back(submodule->clone_as<AstIdentifier>());
    }

    return std::make_unique<AstImport>(
        this->get_source_fragment(),
        this->get_context(),
        this->_package_identifier->clone_as<AstIdentifier>(),
        std::move(cloned_submodules)
    );
}

std::string AstImport::to_string()
{
    std::vector<std::string> modules;
    modules.reserve(this->_import_list.size());

    for (const auto& import_entry : this->_import_list)
    {
        modules.push_back(import_entry->get_scoped_name());
    }
    return std::format(
        "Import [{}] {{ {} }}",
        this->get_package_identifier()->get_scoped_name(),
        join(modules, ", "));
};
