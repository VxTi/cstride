#include "ast/casting.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <llvm/IR/DerivedTypes.h>

using namespace stride::ast;

bool is_array_notation(const TokenSet& set)
{
    return set.peek_eq(TokenType::LSQUARE_BRACKET, 0)
        && set.peek_eq(TokenType::RSQUARE_BRACKET, 1);
}

std::unique_ptr<IAstType> stride::ast::parse_type_metadata(
    std::unique_ptr<IAstType> base_type,
    TokenSet& set
)
{
    int base_flags = base_type->get_flags();
    const auto src_pos = set.peek_next().get_source_position();
    int offset = 0;

    while (is_array_notation(set))
    {
        offset += 2;
        set.skip(2);
        base_type = std::make_unique<AstArrayType>(
            SourcePosition(set.get_source_file(), src_pos.offset, src_pos.length + offset),
            std::move(base_type),
            0
        );
    }

    // If the preceding token is a question mark, the type is determined
    // to be optional.
    // An example of this would be `i32?` or `i32[]?`
    if (set.peek_next_eq(TokenType::QUESTION))
    {
        set.skip(1);
        base_flags |= SRFLAG_TYPE_OPTIONAL;
    }

    base_type->set_flags(base_flags);

    return std::move(base_type);
}

std::unique_ptr<IAstType> AstArrayType::clone()
{
    return std::make_unique<AstArrayType>(
        this->get_source_position(),
        this->_element_type->clone(),
        this->_initial_length,
        this->get_flags()
    );
}

std::string AstArrayType::get_type_name()
{
    if (cast_type<AstFunctionType*>(this->get_element_type()))
    {
        return std::format("({})[]", this->_element_type->get_type_name());
    }
    return std::format("{}[]", this->_element_type->get_type_name());
}

bool AstArrayType::equals(SymbolTable* symbol_table, IAstType* other)
{
    if (const auto* other_array = cast_type<AstArrayType*>(other))
    {
        return this->_element_type->equals(symbol_table, other_array->_element_type.get());
    }

    if (auto* other_named = cast_type<AstAliasType*>(other))
    {
        return other_named->equals(symbol_table, this);
    }

    return false;
}

bool AstArrayType::is_assignable_to_impl(SymbolTable* symbol_table, IAstType* other)
{
    // If we're trying to assign a named type to an array, we have to check
    // whether the referencing type is assignable to this array's element type,
    // e.g., for `type SomeArray = [1, 2, 3]`, `equals(i32[], SomeArray)` should check
    // whether `[1, 2, 3]` in `SomeArray` (i32[]) is assignable to `Array(i32)`
    if (auto* other_alias_ty = cast_type<AstAliasType*>(other))
    {
        const auto reference_type = other_alias_ty->get_primitive_base_type(symbol_table);

        // Validate whether the reference type of `other_named` is assignable to
        return this->is_assignable_to(symbol_table, reference_type);
    }

    // If both are arrays, we can just simply check whether their element types are equal
    // This is handled in the `equals` case.
    return this->equals(symbol_table, other);
}

llvm::Type* AstArrayType::get_llvm_type_impl(SymbolTable* symbol_table, llvm::Module* module)
{
    llvm::Type* element_type = this->get_element_type()->get_llvm_type(symbol_table, module);

    return llvm::ArrayType::get(element_type, this->_initial_length);
}

bool AstArrayType::is_castable_to_impl(SymbolTable* symbol_table, IAstType* other)
{
    // If we're trying to cast an array to a named type, we have to check
    // whether the referencing type is assignable to this array's element type,
    // e.g., for `type SomeArray = [1, 2, 3]`, `equals(i32[], SomeArray)` should check
    // whether `[1, 2, 3]` in `SomeArray` (i32[]) is assignable to `Array(i32)`
    if (auto* other_alias_ty = cast_type<AstAliasType*>(other))
    {
        return this->is_castable_to(symbol_table, other_alias_ty->get_primitive_base_type(symbol_table));
    }

    return false;
}
