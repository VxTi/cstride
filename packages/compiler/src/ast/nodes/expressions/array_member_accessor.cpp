#include "errors.h"
#include "ast/casting.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::unique_ptr<IAstExpression> stride::ast::parse_array_member_accessor(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<AstIdentifier> array_identifier)
{
    auto expression_block = collect_block_variant(
        set,
        TokenType::LSQUARE_BRACKET,
        TokenType::RSQUARE_BRACKET
    );

    if (!expression_block.has_value())
    {
        set.throw_error("Expected array index accessor after '['");
    }

    // If `expression_block` has content, we can safely access `set.peek(-1)` for the source fragment of the closing ']'.
    const auto last_src_pos = set.peek(-1).get_source_fragment();

    auto index_expression = parse_inline_expression(context, expression_block.value());

    const auto source_pos = SourceFragment::combine(array_identifier->get_source_fragment(), last_src_pos);

    return std::make_unique<AstArrayMemberAccessor>(
        source_pos,
        context,
        std::move(array_identifier),
        std::move(index_expression)
    );
}

void AstArrayMemberAccessor::validate()
{
    this->_array_identifier->validate();
    this->_index_accessor_expr->validate();

    const auto index_accessor_type = this->_index_accessor_expr->get_type();

    if (const auto primitive_type = cast_type<AstPrimitiveType*>(index_accessor_type))
    {
        if (!primitive_type->is_integer_ty())
        {
            throw parsing_error(
                ErrorType::SEMANTIC_ERROR,
                std::format(
                    "Array index accessor must be of type int, got '{}'",
                    primitive_type->to_string()),
                this->get_source_fragment());
        }

        return;
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        std::format(
            "Array index accessor must be of type int, got '{}'",
            index_accessor_type->to_string()),
        this->get_source_fragment()
    );
}

llvm::Value* AstArrayMemberAccessor::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    std::unique_ptr<IAstType> array_iden_type = this->_array_identifier->get_type()->clone_ty();

    if (const auto named_ty = cast_type<AstAliasType*>(array_iden_type.get()))
    {
        if (const auto reference_type = named_ty->get_underlying_type();
            reference_type.has_value())
        {
            array_iden_type = reference_type.value()->clone_ty();
        }
        else
        {
            throw parsing_error(
                ErrorType::SEMANTIC_ERROR,
                "Array member accessor used on non-reference type",
                this->get_source_fragment()
            );
        }
    }

    llvm::Value* base_ptr = this->_array_identifier->codegen(
        module,
        builder
    );
    llvm::Value* index_val = this->_index_accessor_expr->codegen(
        module,
        builder
    );

    // Element type, not the array type.
    // Assumes `array_iden_type` is something like "T[]" and has an element type you can extract.
    const auto* array_ty = cast_type<AstArrayType*>(array_iden_type.get());
    if (!array_ty)
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            "Array member accessor used on non-array type",
            this->get_source_fragment());
    }

    llvm::Type* elem_llvm_ty = type_to_llvm_type(
        array_ty->get_element_type(),
        module);

    // Treat base_ptr as elem*
    llvm::Value* typed_base_ptr = builder->CreateBitCast(
        base_ptr,
        llvm::PointerType::getUnqual(module->getContext()),
        "array_base_cast"
    );

    llvm::Value* element_ptr = builder->CreateInBoundsGEP(
        elem_llvm_ty,
        typed_base_ptr,
        index_val,
        "array_elem_ptr"
    );

    return builder->CreateLoad(elem_llvm_ty, element_ptr, "array_load");
}

std::unique_ptr<IAstNode> AstArrayMemberAccessor::clone()
{
    return std::make_unique<AstArrayMemberAccessor>(
        this->get_source_fragment(),
        this->get_context(),
        this->_array_identifier->clone_as<AstIdentifier>(),
        this->_index_accessor_expr->clone_as<IAstExpression>()
    );
}

std::string AstArrayMemberAccessor::to_string()
{
    return std::format(
        "ArrayAccess({}, {})",
        this->_array_identifier->to_string(),
        this->_index_accessor_expr->to_string()
    );
}

bool AstArrayMemberAccessor::is_reducible()
{
    // If the value is a literal, it's reducible for sure.
    if (cast_expr<AstLiteral*>(this->_array_identifier.get()))
    {
        return true;
    }

    // Otherwise, we'll perhaps be able to reduce it if the index accessor expression itself
    // is reducible.
    return this->_index_accessor_expr->is_reducible();
}

std::optional<std::unique_ptr<IAstNode>> AstArrayMemberAccessor::reduce()
{
    return std::nullopt;
}
