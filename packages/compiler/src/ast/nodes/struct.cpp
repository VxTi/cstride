#include <sstream>

#include "ast/nodes/struct.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;

std::unique_ptr<AstStructMember> parse_struct_member(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto struct_member_name_tok = set.expect(TokenType::IDENTIFIER, "Expected struct member name");
    const auto struct_member_name = struct_member_name_tok.lexeme;

    set.expect(TokenType::COLON);

    auto struct_member_type = parse_type(scope, set, "Expected struct member type");
    set.expect(TokenType::SEMICOLON);

    // TODO: Replace with struct-field-specific method
    scope->define_field(struct_member_name, struct_member_name, struct_member_type->clone());

    return std::make_unique<AstStructMember>(
        set.source(),
        struct_member_name_tok.offset,
        scope,
        struct_member_name,
        std::move(struct_member_type)
    );
}

bool stride::ast::is_struct_declaration(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_STRUCT);
}

std::unique_ptr<AstStruct> stride::ast::parse_struct_declaration(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& tokens
)
{
    if (scope->get_current_scope() != ScopeType::GLOBAL && scope->get_current_scope() != ScopeType::MODULE)
    {
        tokens.throw_error("Struct declarations are only allowed in global or module scope");
    }

    const auto reference_token = tokens.expect(TokenType::KEYWORD_STRUCT);
    const auto struct_name_tok = tokens.expect(TokenType::IDENTIFIER, "Expected struct name");
    const auto struct_name = struct_name_tok.lexeme;

    // Might be a reference to another type
    // This will parse a definition like:
    //
    // struct Foo = Bar;
    if (tokens.peak_next_eq(TokenType::EQUALS))
    {
        tokens.next();
        auto reference_sym = parse_type(scope, tokens, "Expected reference struct type", SRFLAG_NONE);
        tokens.expect(TokenType::SEMICOLON);

        // We define it as a reference to `reference_sym`. Validation happens later
        scope->define_struct(struct_name, reference_sym->get_internal_name());

        return std::make_unique<AstStruct>(
            tokens.source(),
            reference_token.offset,
            scope,
            struct_name,
            std::move(reference_sym)
        );
    }

    auto struct_body_set = collect_block(tokens);
    std::vector<std::unique_ptr<AstStructMember>> members = {};
    std::vector<std::unique_ptr<IAstInternalFieldType>> fields = {};

    if (struct_body_set.has_value())
    {
        auto nested_scope = std::make_shared<SymbolRegistry>(scope, ScopeType::BLOCK);
        while (struct_body_set.value().has_next())
        {
            auto member = parse_struct_member(nested_scope, struct_body_set.value());

            fields.push_back(member->get_type().clone());
            members.push_back(std::move(member));
        }
    }

    scope->define_struct(struct_name, std::move(fields));

    return std::make_unique<AstStruct>(
        tokens.source(),
        reference_token.offset,
        scope,
        struct_name,
        std::move(members)
    );
}

void AstStructMember::validate()
{
    if (const auto non_primitive_type = dynamic_cast<AstNamedValueType*>(this->_type.get()))
    {
        // If it's not a primitive, it must be a struct, as we currently don't support other types.
        // TODO: Support enums

        if (const auto member = this->scope->get_struct_def(non_primitive_type->get_internal_name()); member == nullptr)
        {
            throw parsing_error(
                make_ast_error(
                    *this->source,
                    this->source_offset,
                    std::format("Undefined struct type for member '{}'", this->get_name()))
            );
        }
    }
}

void AstStruct::validate()
{
    // If it's a reference type, it must at least exist...
    if (this->is_reference_type())
    {
        // Check whether we can find the other field
        if (const auto reference = this->scope->get_struct_def(this->get_reference_type()->get_internal_name());
            reference == nullptr)
        {
            throw parsing_error(
                make_source_error(
                    *this->source,
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "Unable to determine type of struct '{}': referenced struct type '{}' is undefined",
                        this->get_name(),
                        this->_reference.value()->get_internal_name()
                    ),
                    this->_reference.value()->source_offset,
                    this->_reference.value()->get_internal_name().length()
                )
            );
        }
        return;
    }

    // Alright, now we'll have to validate all fields instead...
    for (const auto& member : this->get_members())
    {
        member->validate();
    }
}

std::string AstStructMember::to_string()
{
    return std::format("{}: {}", this->get_name(), this->get_type().to_string());
}

std::string AstStruct::to_string()
{
    if (this->is_reference_type())
    {
        return std::format("Struct({}) (reference to {})", this->get_name(), this->get_reference_type()->to_string());
    }
    if (this->get_members().empty())
    {
        return std::format("Struct({}) (empty)", this->get_name());
    }

    std::ostringstream imploded;

    imploded << this->get_members()[0]->to_string();
    for (size_t i = 1; i < this->get_members().size(); ++i)
    {
        imploded << "\n  " << this->get_members()[i]->to_string();
    }
    return std::format("Struct({}) (\n  {}\n)", this->get_name(), imploded.str());
}
