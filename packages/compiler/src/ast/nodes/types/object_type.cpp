#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <ranges>
#include <llvm/IR/Module.h>

using namespace stride::ast;

void parse_object_member(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    ObjectTypeMemberList& fields
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
 *   field: i32,
 *   ...
 * }
 * </code>
 */
std::optional<std::unique_ptr<IAstType>> stride::ast::parse_object_type_optional(
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

    const auto reference_token = set.peek_next();

    auto struct_body_set = collect_block_required(set, "A struct must have at least 1 member");

    ObjectTypeMemberList struct_fields = {};
    const auto struct_type_context = std::make_shared<ParsingContext>(
        context,
        context->get_context_type());

    // Parse fields
    while (struct_body_set.has_next())
    {
        parse_object_member(struct_type_context, struct_body_set, struct_fields);
    }

    // Re-verification
    if (struct_fields.empty())
    {
        set.throw_error("Struct must have at least one member");
    }

    auto struct_ty = std::make_unique<AstObjectType>(
        reference_token.get_source_fragment(),
        struct_type_context,
        std::move(struct_fields),
        context_type_flags
    );

    return parse_type_metadata(std::move(struct_ty), set, context_type_flags);
}

std::optional<IAstType*> AstObjectType::get_member_field_type(const std::string& field_name) const
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

std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> AstObjectType::get_members() const
{
    std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> members;
    members.reserve(this->_members.size());

    for (const auto& [name, type] : this->_members)
    {
        members.emplace_back(name, type->clone_ty());
    }

    return std::move(members);
}

std::optional<int> AstObjectType::get_member_field_index(const std::string& field_name) const
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

/// Produces a name based on the field types,
/// so that all structs with the same fields have the same name,
/// resulting in no LLVM duplication
std::string AstObjectType::get_internalized_name() const
{
    if (!this->_base_name.empty())
    {
        if (this->_instantiated_generics.empty())
        {
            return this->_base_name;
        }

        std::string name = this->_base_name;
        for (const auto& gen : this->_instantiated_generics)
        {
            name += "_" + gen->get_type_name();
        }
        return name;
    }

    std::string internalized_name;
    for (const auto& type : this->_members | std::views::values)
    {
        if (const auto struct_ty = cast_type<AstObjectType*>(type.get()))
        {
            // Ensure nested structs are also appended properly
            internalized_name += "_" + struct_ty->get_internalized_name();
        }
        else
        {
            internalized_name += "_" + type->get_type_name();
        }
    }
    return std::format("struct${:x}", std::hash<std::string>{}(internalized_name));
}

void AstObjectType::resolve_forward_references(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    // Retrieve or Create the named struct type (Opaque)
    // We check if it already exists to support forward declarations or multi-pass compilation.
    const auto internal_name = this->get_internalized_name();
    llvm::StructType* struct_type = llvm::StructType::getTypeByName(
        module->getContext(),
        internal_name
    );
    if (!struct_type)
    {
        struct_type = llvm::StructType::create(module->getContext(), internal_name);
    }

    // If the body is already defined, we stop here to avoid re-definition errors.
    if (!struct_type->isOpaque())
    {
        return;
    }

    std::vector<llvm::Type*> member_types;
    member_types.reserve(this->get_members().size());

    // Case: type Name { members... }
    for (const auto& [member_name, member_type] : this->get_members())
    {
        llvm::Type* llvm_type = type_to_llvm_type(member_type.get(), module);

        if (!llvm_type)
        {
            // Note: If you support pointers to self (recursive structs), usually
            // you would find the opaque type created in step 1 here.
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Unknown internal type '{}' for struct member '{}'",
                    member_type->get_type_name(),
                    member_name
                ),
                member_type->get_source_fragment()
            );
        }

        member_types.push_back(llvm_type);
    }

    // Set the Body
    // This finalizes the layout of the struct.
    // 'isPacked' is false by default unless you have specific packing rules.
    struct_type->setBody(member_types, /*isPacked=*/false);
}

bool AstObjectType::equals(IAstType* other)
{
    if (const auto other_struct_ty = cast_type<const AstObjectType*>(other))
    {
        if (this->_members.size() != other_struct_ty->_members.size())
        {
            return false;
        }

        for (size_t i = 0; i < this->_members.size(); i++)
        {
            const auto& [first_field_name, first_type] = this->_members[i];
            const auto& [second_field_name, second_type] = other_struct_ty->_members[i];

            if (first_field_name != second_field_name)
            {
                return false;
            }

            // Optimization: if both types are named types with the same name, they are equal, 
            // even if their underlying type is not yet resolvable (e.g. generic parameter T).
            const auto* first_alias = cast_type<AstAliasType*>(first_type.get());
            const auto* second_alias = cast_type<AstAliasType*>(second_type.get());
            if (first_alias && second_alias && first_alias->get_name() == second_alias->get_name())
            {
                continue;
            }

            if (!first_type.get()->equals(second_type.get()))
            {
                return false;
            }
        }

        return true;
    }

    if (auto* other_named = cast_type<AstAliasType*>(other))
    {
        return other_named->equals(this);
    }

    return false;
}

std::unique_ptr<IAstNode> AstObjectType::clone()
{
    ObjectTypeMemberList cloned_members;
    cloned_members.reserve(this->_members.size());

    for (const auto& [name, type] : this->_members)
    {
        cloned_members.emplace_back(name, type->clone_ty());
    }

    GenericTypeList cloned_generics;
    cloned_generics.reserve(this->_instantiated_generics.size());
    for (const auto& gen : this->_instantiated_generics)
    {
        cloned_generics.push_back(gen->clone_ty());
    }

    return std::make_unique<AstObjectType>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(cloned_members),
        this->get_flags(),
        this->_base_name,
        std::move(cloned_generics)
    );
}
