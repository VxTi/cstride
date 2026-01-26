#include "ast/nodes/package.h"

#include <format>

using namespace stride::ast;

void AstPackage::validate() {}

std::string AstPackage::to_string()
{
    return std::format("Package({})", this->get_name());
}

bool stride::ast::is_package_declaration(const TokenSet& set)
{
    return set.peak_next_eq(TokenType::KEYWORD_PACKAGE);
}

std::unique_ptr<AstPackage> stride::ast::parse_package_declaration(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const int initial_offset = set.position();
    const auto reference_token = set.expect(TokenType::KEYWORD_PACKAGE);
    const auto package_name = set.expect(TokenType::IDENTIFIER, "Expected package name").lexeme;

    if (initial_offset != 0)
    {
        set.throw_error("Package declarations must be at the top of the file");
    }

    set.expect(TokenType::SEMICOLON, "Expected semicolon after package declaration");

    return std::make_unique<AstPackage>(
        set.source(),
        reference_token.offset,
        scope,
        package_name
    );
}
