#include "ast/casting.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_tuple_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    const TypeParsingOptions& options
)
{
    // Tuples must be in the form of (T0, T1, ...), where Tn can be any type.
    // Member types are also allowed to be tuples, so ((i8, i32), i32) is allowed.
    if (!set.peek_next_eq(TokenType::LPAREN))
        return std::nullopt;

    const auto& reference_token = set.peek_next();
    auto type_set = collect_parenthesized_block(set);

    std::vector<std::unique_ptr<IAstType>> members;

    if (!type_set.has_value() || type_set->size() == 0)
    {
        set.throw_error("Expected a sequence of types in tuple declaration");
    }

    members.push_back(
        parse_type(
            context,
            type_set.value(),
            { "Expected types in tuple type declaration" }
        )
    );

    while (set.peek_next_eq(TokenType::COMMA))
    {
        set.next();
        members.push_back(
            parse_type(
                context,
                type_set.value(),
                { "Expected types in tuple type declaration" }
            )
        );
    }

    const auto last_pos = type_set.has_value()
        ? type_set->last().get_source_fragment()
        : reference_token.get_source_fragment();

    const auto source = SourceFragment(
        set.get_source(),
        reference_token.get_source_fragment().offset,
        last_pos.offset + last_pos.length - reference_token.get_source_fragment().offset
    );

    return std::make_unique<AstTupleType>(
        source,
        context,
        std::move(members),
        options.flags
    );
}

llvm::Value* AstTupleType::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return nullptr;
}

llvm::Type* AstTupleType::get_llvm_type_impl(llvm::Module* module)
{
    std::vector<llvm::Type*> member_llvm_types;
    member_llvm_types.reserve(this->_members.size());

    for (const auto& member : this->_members)
    {
        member_llvm_types.push_back(member->get_llvm_type(module));
    }

    return llvm::StructType::get(module->getContext(), member_llvm_types);
}

std::unique_ptr<IAstNode> AstTupleType::clone()
{
    std::vector<std::unique_ptr<IAstType>> members;
    members.reserve(this->_members.size());
    for (const auto& member : this->_members)
    {
        members.push_back(member->clone_ty());
    }

    return std::make_unique<AstTupleType>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(members),
        this->get_flags()
    );
}

bool AstTupleType::equals(IAstType* other)
{
    const auto other_tuple = cast_type<AstTupleType*>(other);

    if (!other_tuple)
    {
        if (auto* other_named = cast_type<AstAliasType*>(other))
        {
            return other_named->equals(this);
        }
        return false; // other type is not a tuple; no equality
    }

    // Member count must be equal
    if (this->_members.size() != other_tuple->_members.size())
        return false;

    for (size_t i = 0; i < this->_members.size(); ++i)
    {
        if (!this->_members[i]->equals(other_tuple->_members[i].get()))
        {
            return false; // Found a pair of members that are not equal
        }
    }
    return true;
}

std::string AstTupleType::to_string()
{
    std::vector<std::string> member_strings;
    member_strings.reserve(this->_members.size());

    for (const auto& member : this->_members)
    {
        member_strings.push_back(member->to_string());
    }

    return std::format("({})", join(member_strings, ", "));
}
