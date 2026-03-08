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
std::vector<Symbol> consume_import_submodules(TokenSet& set)
{
    set.expect(TokenType::DOUBLE_COLON, "Expected a '::' before import submodule list");
    set.expect(TokenType::LBRACE, "Expected opening brace with modules after '::'");

    const auto initial_import_reference_token = set.peek(0);
    const auto initial_import_segments = parse_segmented_identifier(
        set,
        "Expected module name after '::' in import list"
    );
    const auto initial_import_name = resolve_internal_name(initial_import_segments);

    std::vector submodules = {
        Symbol(initial_import_reference_token.get_source_fragment(), initial_import_name)
    };

    while (set.peek(0) == TokenType::COMMA && set.peek(1) ==
        TokenType::IDENTIFIER)
    {
        set.expect(TokenType::COMMA, "Expected comma between module names in import list");
        const auto submodule_reference_token = set.peek(0);
        const auto submodule_identifier_segments = parse_segmented_identifier(
            set,
            "Expected module name after '::' in import list"
        );
        const auto submodule_iden = resolve_internal_name(
            submodule_identifier_segments
        );

        // TODO: Fix source fragment positioning; currently only base identifier instead of whole
        submodules.emplace_back(
            submodule_reference_token.get_source_fragment(),
            submodule_iden
        );
    }

    set.expect(TokenType::RBRACE, "Expected closing brace after import list");
    set.expect(TokenType::SEMICOLON, "Expected semicolon after import list");

    if (submodules.empty())
    {
        set.throw_error("Expected at least one symbol in import submodule list");
    }

    return submodules;
}

/**
 * Attempts to parse an import expression from the given TokenSet.
 */
std::unique_ptr<AstImport> stride::ast::parse_import_statement(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_IMPORT);
    // With the guard clause, this will always be the case.

    const auto import_package_segments = parse_segmented_identifier(
        set,
        "Expected module name after '::'");

    const auto import_module = resolve_internal_name(
        context->get_name(),
        reference_token.get_source_fragment(),
        import_package_segments
    );
    const auto import_list = consume_import_submodules(set);

    const Dependency dependency = {
        .package_name = import_module,
        .submodules  = import_list
    };

    return std::make_unique<AstImport>(
        reference_token.get_source_fragment(),
        context,
        dependency
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
    return std::make_unique<AstImport>(*this);
}

std::string AstImport::to_string()
{
    std::vector<std::string> modules;
    modules.reserve(this->get_submodules().size());

    for (const auto& module : this->get_submodules())
    {
        modules.push_back(module.name);
    }
    return std::format(
        "Import [{}] {{ {} }}",
        this->get_module().name,
        join(modules, ", "));
};
