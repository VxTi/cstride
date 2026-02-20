#include <sstream>

#include "ast/nodes/struct.h"

#include <llvm/IR/Module.h>

#include "ast/casting.h"
#include "ast/symbols.h"
#include "ast/nodes/blocks.h"

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<AstStructMember> parse_struct_member(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto struct_member_name_tok = set.expect(TokenType::IDENTIFIER, "Expected struct member name");
    const auto& struct_member_name = struct_member_name_tok.get_lexeme();

    set.expect(TokenType::COLON);

    auto struct_member_type = parse_type(context, set, "Expected a struct member type");
    const auto last_token = set.expect(TokenType::SEMICOLON, "Expected ';' after struct member declaration");

    const auto position = stride::SourceLocation(
        set.get_source(),
        struct_member_name_tok.get_source_position().offset,
        last_token.get_source_position().offset
        + last_token.get_source_position().length
        - struct_member_name_tok.get_source_position().offset
    );
    auto struct_member_symbol = Symbol(position, struct_member_name);

    // TODO: Replace with struct-field-specific method
    context->define_variable(
        struct_member_symbol,
        struct_member_type->clone()
    );

    return std::make_unique<AstStructMember>(
        context,
        struct_member_symbol,
        std::move(struct_member_type)
    );
}

std::unique_ptr<AstStruct> stride::ast::parse_struct_declaration(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& tokens,
    VisibilityModifier modifier
)
{
    if (context->get_current_scope_type() != ScopeType::GLOBAL && context->get_current_scope_type() !=
        ScopeType::MODULE)
    {
        tokens.throw_error("Struct declarations are only allowed in global or module scope");
    }

    const auto reference_token = tokens.expect(TokenType::KEYWORD_STRUCT);
    const auto struct_name_tok = tokens.expect(TokenType::IDENTIFIER, "Expected struct name");
    const auto& struct_name = struct_name_tok.get_lexeme();

    // Might be a reference to another type
    // This will parse a definition like:
    //
    // struct Foo = Bar;
    /// -- returns --
    if (tokens.peek_next_eq(TokenType::EQUALS))
    {
        tokens.next();
        auto reference_sym = parse_type(context, tokens, "Expected reference struct type", SRFLAG_NONE);
        tokens.expect(TokenType::SEMICOLON);

        // We define it as a reference to `reference_sym`. Validation happens later
        context->define_struct(
            /* Base struct sym */
            Symbol(struct_name_tok.get_source_position(), struct_name),
            /* Reference struct sym */
            Symbol(reference_sym->get_source_position(), reference_sym->get_internal_name())
        );

        return std::make_unique<AstStruct>(
            reference_token.get_source_position(),
            context,
            struct_name,
            std::move(reference_sym)
        );
    }

    auto struct_body_set = collect_block(tokens);

    // Ensure we have at least one member in the struct body
    if (!struct_body_set.has_value() || !struct_body_set.value().has_next())
    {
        const auto ref_src_pos = reference_token.get_source_position();
        const auto struct_name_pos = struct_name_tok.get_source_position();

        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            "A struct must have at least 1 member",
            SourceLocation(
                tokens.get_source(),
                ref_src_pos.offset,
                struct_name_pos.offset + struct_name_pos.length - ref_src_pos.offset
            )
        );
    }

    std::vector<std::unique_ptr<AstStructMember>> members = {};
    std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> fields = {};

    if (struct_body_set.has_value())
    {
        const auto nested_scope = std::make_shared<ParsingContext>(context, ScopeType::BLOCK);
        while (struct_body_set.value().has_next())
        {
            auto member = parse_struct_member(nested_scope, struct_body_set.value());

            fields.emplace_back(member->get_name(), member->get_type().clone());
            members.push_back(std::move(member));
        }
    }

    // Re-verification
    if (members.empty())
    {
        tokens.throw_error("Struct must have at least one member");
    }

    context->define_struct(
        Symbol(struct_name_tok.get_source_position(), struct_name),
        std::move(fields)
    );

    return std::make_unique<AstStruct>(
        reference_token.get_source_position(),
        context,
        struct_name,
        std::move(members)
    );
}

void AstStructMember::validate()
{
    if (const auto struct_type = cast_type<AstNamedType*>(this->_type.get()))
    {
        // If it's not a primitive, it must be a struct, as we currently don't support other types.
        // TODO: Support enums

        if (const auto member = this->get_context()->get_struct_def(struct_type->get_internal_name());
            !member.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Undefined struct type for member '{}'", this->get_name()),
                this->get_source_position()
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
        if (const auto reference = this->get_context()->get_struct_def(this->get_reference_type()->get_internal_name())
            ;
            reference == nullptr)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Unable to determine type of struct '{}': referenced struct type '{}' is undefined",
                    this->get_name(),
                    this->_reference.value()->get_internal_name()
                ),
                this->_reference.value()->get_source_position()
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

void AstStruct::resolve_forward_references(const std::shared_ptr<ParsingContext>& context, llvm::Module* module,
                                           llvm::IRBuilder<>* builder)
{
    // Retrieve or Create the named struct type (Opaque)
    // We check if it already exists to support forward declarations or multi-pass compilation.
    llvm::StructType* struct_type = llvm::StructType::getTypeByName(module->getContext(), this->get_name());
    if (!struct_type)
    {
        struct_type = llvm::StructType::create(module->getContext(), this->get_name());
    }

    // If the body is already defined, we stop here to avoid re-definition errors.
    if (!struct_type->isOpaque())
    {
        return;
    }

    std::vector<llvm::Type*> member_types;

    //  Resolve Member Types
    if (this->is_reference_type())
    {
        // Case: struct Alias = Original;
        // We look up the original struct and copy its layout.
        // TODO: Mangle ref name
        std::string ref_name = this->get_reference_type()->get_internal_name();
        const llvm::StructType* ref_struct = llvm::StructType::getTypeByName(module->getContext(), ref_name);

        if (!ref_struct)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Referenced struct type '{}' not found during codegen", ref_name),
                this->get_source_position()
            );
        }

        // If the referenced struct is still opaque/incomplete, we cannot alias it yet.
        if (ref_struct->isOpaque())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Referenced struct type '{}' is not fully defined", ref_name),
                this->get_source_position()
            );
        }

        member_types = ref_struct->elements();
    }
    else
    {
        // Case: struct Name { members... }
        for (const auto& member : this->get_members())
        {
            llvm::Type* llvm_type = internal_type_to_llvm_type(&member->get_type(), module);

            if (!llvm_type)
            {
                // Note: If you support pointers to self (recursive structs), usually
                // you would find the opaque type created in step 1 here.
                throw parsing_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "Unknown internal type '{}' for struct member '{}'",
                        member->get_type().get_internal_name(), member->get_name()
                    ),
                    member->get_source_position()
                );
            }

            member_types.push_back(llvm_type);
        }
    }

    // Set the Body
    // This finalizes the layout of the struct.
    // 'isPacked' is false by default unless you have specific packing rules.
    struct_type->setBody(member_types, /*isPacked=*/false);
}

llvm::Value* AstStruct::codegen(
    const std::shared_ptr<ParsingContext>& context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    // Struct registration is done in the `resolve_forward_references` pass
    // so that one can reference structs before they're semantically defined.
    return nullptr;
}
