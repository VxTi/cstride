#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"
#include "ast/nodes/type_definition.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <ranges>
#include <string>
#include <vector>

using namespace stride::ast;

AstEnumType::AstEnumType(
    const SourceFragment& source,
    const std::shared_ptr<ParsingContext>& context,
    std::string enum_name,
    std::vector<EnumMemberPair> members,
    const int flags
) :
    IAstType(source, context, flags),
    _members(std::move(members)),
    _name(std::move(enum_name)) {}

/**
 * Parses member entries with the following sequence:
 * <code>
 * IDENTIFIER: <literal value>;
 * </code>
 */
EnumMemberPair stride::ast::parse_enumerable_member(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    size_t element_index
)
{
    const auto member_name_tok = set.expect(TokenType::IDENTIFIER);
    auto member_name = member_name_tok.get_lexeme();

    // Using index as element value if no explicit value is provided, and allowing optional trailing comma
    if (!set.has_next() || !set.peek_next_eq(TokenType::COLON))
    {
        auto value = std::make_unique<AstIntLiteral>(
                member_name_tok.get_source_fragment(),
                context,
                PrimitiveType::INT32,
                element_index
            );

        return {
            member_name,
            std::move(value)
        };
    }

    set.expect(TokenType::COLON, "Expected a colon after enum member name");

    auto value = parse_literal_optional(context, set);

    if (!value.has_value())
        set.throw_error("Expected a literal value for enum member");

    return {
        member_name,
        std::move(value.value())
    };
}

std::unique_ptr<AstTypeDefinition> stride::ast::parse_enum_type_definition(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    [[maybe_unused]] VisibilityModifier modifier
)
{
    const auto reference_token = set.expect(TokenType::KEYWORD_ENUM);
    const auto enumerable_name = set.expect(TokenType::IDENTIFIER).get_lexeme();

    auto enum_body_subset = collect_block_required(set, "Expected a block in enum declaration");

    std::vector<EnumMemberPair> members;

    members.push_back(parse_enumerable_member(context, enum_body_subset, 0));

    for (size_t i = 1; enum_body_subset.has_next(); ++i)
    {
        enum_body_subset.expect(TokenType::COMMA, "Expected a comma between enum members");
        members.push_back(parse_enumerable_member(context, enum_body_subset, i));
    }

    auto type = std::make_unique<AstEnumType>(
        reference_token.get_source_fragment(),
        context,
        enumerable_name,
        std::move(members)
    );

    context->define_type(
        Symbol(reference_token.get_source_fragment(),
               context->get_name(),
               enumerable_name),
        type->clone_ty(),
        {},
        modifier
    );

    return std::make_unique<AstTypeDefinition>(
        reference_token.get_source_fragment(),
        context,
        enumerable_name,
        std::move(type),
        modifier
    );
}


std::unique_ptr<IAstNode> AstEnumType::clone()
{
    std::vector<EnumMemberPair> cloned_members;
    cloned_members.reserve(this->_members.size());

    for (const auto& [field_name, field_ty] : this->_members)
    {
        cloned_members.emplace_back(field_name, field_ty->clone());
    }

    return std::make_unique<AstEnumType>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_name(),
        std::move(cloned_members),
        this->get_flags()
    );
}

llvm::Value* AstEnumType::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return nullptr;
}

bool AstEnumType::equals(IAstType* other)
{
    if (const auto other_enum = cast_type<AstEnumType*>(other))
    {
        return this->get_name() == other_enum->get_name();
    }
    return false;
}

llvm::Type* AstEnumType::get_llvm_type_impl(llvm::Module* module)
{
   throw parsing_error(
       ErrorType::COMPILATION_ERROR,
        std::format("Cannot get LLVM type for enum type '{}'", this->get_name()),
        this->get_source_fragment()
    );
}

bool AstEnumType::is_assignable_to_impl(IAstType* other)
{
    return false; // Already managed by `AstType` (equality check)
}

void AstEnumType::resolve_forward_references(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    for (const auto& val : this->_members | std::views::values)
    {
        val->resolve_forward_references(module, builder);
    }
}

std::string AstEnumType::to_string()
{
    std::vector<std::string> members;

    for (const auto& [field_name, field_val] : this->get_members())
    {
        members.push_back(std::format("{} = {}", field_name, field_val->to_string()));
    }

    return std::format(
        "Enumerable {} (\n  {}\n)",
        this->get_name(),
        join(members, ",\n  "));
}
