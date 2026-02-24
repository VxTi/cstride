#include "ast/nodes/blocks.h"
#include "ast/nodes/struct_declaration.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

void parse_struct_member(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::vector<definition::StructFieldPair>& fields
)
{
    const auto struct_member_name_tok = set.expect(
        TokenType::IDENTIFIER,
        "Expected struct member name"
    );
    const auto& struct_member_name = struct_member_name_tok.get_lexeme();

    set.expect(TokenType::COLON);

    auto struct_member_type = parse_type(
        context,
        set,
        "Expected a struct member type"
    );
    const auto last_token = set.expect(
        TokenType::SEMICOLON,
        "Expected ';' after struct member declaration"
    );

    const auto& last_pos = last_token.get_source_fragment();
    const auto& mem_pos = struct_member_name_tok.get_source_fragment();

    const auto position = stride::SourceFragment(
        set.get_source(),
        mem_pos.offset,
        last_pos.offset + last_pos.length - mem_pos.offset
    );

    fields.emplace_back(struct_member_name, std::move(struct_member_type));
}

/**
 * Optionally parses a struct definition.
 * Structures are defined like follows:
 * <code>
 * type Struct = {
 *   field: int32,
 *   ...
 * }
 * </code>
 */
std::optional<std::unique_ptr<IAstType>> stride::ast::parse_struct_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    int context_type_flags
)
{
    // Must start with `{`
    // A struct type is defined like:
    // { field: type, ... }
    if (!set.peek_next_eq(TokenType::LBRACE))
        return std::nullopt;

    const auto reference_token = set.next();

    auto struct_body_set = collect_block(set);

    // Ensure we have at least one member in the struct body
    if (!struct_body_set.has_value() || !struct_body_set.value().has_next())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            "A struct must have at least 1 member",
            reference_token.get_source_fragment());
    }

    std::vector<definition::StructFieldPair> struct_fields = {};

    // Parse fields
    if (struct_body_set.has_value())
    {
        const auto nested_scope = std::make_shared<ParsingContext>(
            context,
            definition::ScopeType::BLOCK);
        while (struct_body_set.value().has_next())
        {
            parse_struct_member(
                nested_scope,
                struct_body_set.value(),
                struct_fields
            );
        }
    }

    // Re-verification
    if (struct_fields.empty())
    {
        set.throw_error("Struct must have at least one member");
    }

    return std::make_unique<AstStructType>(
        reference_token.get_source_fragment(),
        context,
        std::move(struct_fields),
        context_type_flags
    );
}

std::optional<const IAstType*> AstStructType::get_member_field_type(const std::string& field_name) const
{
    for (const auto& [name, type] : this->_members)
    {
        if (name == field_name)
        {
            return type.get();
        }
    }
    return std::nullopt;
}

std::optional<int> AstStructType::get_member_field_index(const std::string& field_name) const
{
    for (size_t i = 0; i < this->_members.size(); i++)
    {
        if (this->_members[i].first == field_name)
        {
            return static_cast<int>(i);
        }
    }
    return std::nullopt;
}

bool AstStructType::equals(IAstType& other)
{
    if (const auto other_struct_ty = cast_type<AstStructType*>(&other))
    {
        if (this->get_members().size() != other_struct_ty->get_members().size())
        {
            return false;
        }

        for (size_t i = 0; i < this->get_members().size(); i++)
        {
            const auto& [first_field_name, first_type] = this->get_members()[i];
            const auto& [second_field_name, second_type] = other_struct_ty->get_members()[i];

            if (first_field_name != second_field_name ||
                !first_type.get()->equals(*second_type.get()))
            {
                return false;
            }
        }

        return true;

    }
    return false;
}

std::unique_ptr<IAstType> AstStructType::clone() const
{
    std::vector<definition::StructFieldPair> cloned_members = {};

    for (const auto& [name, type] : this->get_members())
    {
        cloned_members.emplace_back(name, type->clone());
    }

    return std::make_unique<AstStructType>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(cloned_members),
        this->get_flags()
    );
}
