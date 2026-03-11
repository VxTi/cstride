#include "ast/nodes/enumerables.h"

#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <sstream>

using namespace stride::ast;
using namespace stride::ast::definition;

/**
 * Parses member entries with the following sequence:
 * <code>
 * IDENTIFIER: <literal value>;
 * </code>
 */
std::unique_ptr<AstEnumerableMember> stride::ast::parse_enumerable_member(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    size_t element_index
)
{
    const auto member_name_tok = set.expect(TokenType::IDENTIFIER);
    auto member_sym = member_name_tok.get_lexeme();

    context->define_symbol(
        Symbol(
            member_name_tok.get_source_fragment(),
            context->get_name(),
            member_sym
        ),
        SymbolType::ENUM_MEMBER
    );

    if (!set.has_next() || !set.peek_next_eq(TokenType::COLON))
    {
        // Using index as element value
        if (set.has_next() && set.peek_next_eq(TokenType::COMMA))
        {
            // Consume optional trailing comma
            set.next();
        }

        return std::make_unique<AstEnumerableMember>(
            member_name_tok.get_source_fragment(),
            context,
            std::move(member_sym),
            std::make_unique<AstIntLiteral>(
                member_name_tok.get_source_fragment(),
                context,
                PrimitiveType::INT32,
                element_index
            )
        );
    }

    set.expect(TokenType::COLON, "Expected a colon after enum member name");

    auto value = parse_literal_optional(context, set);

    if (!value.has_value())
    {
        set.throw_error("Expected a literal value for enum member");
    }

    set.expect(TokenType::COMMA, "Expected a comma after enum member value");

    return std::make_unique<AstEnumerableMember>(
        member_name_tok.get_source_fragment(),
        context,
        std::move(member_sym),
        std::move(value.value())
    );
}

std::unique_ptr<AstEnumerable> stride::ast::parse_enumerable_declaration(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    [[maybe_unused]] VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_ENUM);
    const auto enumerable_name_tok = set.expect(TokenType::IDENTIFIER);
    auto enumerable_name = enumerable_name_tok.get_lexeme();

    context->define_symbol(
        Symbol(reference_token.get_source_fragment(),
               context->get_name(),
               enumerable_name),
        SymbolType::ENUM
    );

    auto enum_body_subset = collect_block_required(set, "Expected a block in enum declaration");

    std::vector<std::unique_ptr<AstEnumerableMember>> members = {};

    auto enum_definition_context = std::make_shared<ParsingContext>(
        context,
        context->get_context_type());

    for (size_t i = 0; enum_body_subset.has_next(); ++i)
    {
        members.push_back(parse_enumerable_member(enum_definition_context, enum_body_subset, i));
    }

    return std::make_unique<AstEnumerable>(
        reference_token.get_source_fragment(),
        enum_definition_context,
        std::move(members),
        enumerable_name
    );
}

std::unique_ptr<IAstNode> AstEnumerable::clone()
{
    std::vector<std::unique_ptr<AstEnumerableMember>> cloned_members;
    cloned_members.reserve(this->get_members().size());

    for (const auto& member : this->get_members())
    {
        cloned_members.push_back(member->clone_as<AstEnumerableMember>());
    }

    return std::make_unique<AstEnumerable>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(cloned_members),
        this->get_name()
    );
}

std::unique_ptr<IAstNode> AstEnumerableMember::clone()
{
    return std::make_unique<AstEnumerableMember>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_name(),
        this->value().clone_as<AstLiteral>()
    );
}

std::string AstEnumerableMember::to_string()
{
    auto member_value_str = this->value().to_string();
    return std::format("{}: {}", this->get_name(), member_value_str);
}

std::string AstEnumerable::to_string()
{
    std::ostringstream imploded;

    if (this->get_members().empty())
    {
        return std::format("Enumerable {} (empty)", this->get_name());
    }

    imploded << this->get_members()[0]->to_string();
    for (size_t i = 1; i < this->get_members().size(); ++i)
    {
        imploded << "\n  " << this->get_members()[i]->to_string();
    }

    return std::format("Enumerable {} (\n  {}\n)",
                       this->get_name(),
                       imploded.str());
}
