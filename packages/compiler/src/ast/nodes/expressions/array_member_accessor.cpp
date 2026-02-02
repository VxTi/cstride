#include <llvm/IR/Module.h>

#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::unique_ptr<AstExpression> stride::ast::parse_array_member_accessor(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set,
    std::unique_ptr<AstIdentifier> array_identifier
)
{
    auto expression_block = collect_block_variant(set, TokenType::LSQUARE_BRACKET, TokenType::RSQUARE_BRACKET);

    if (!expression_block.has_value())
    {
        set.throw_error("Expected array index accessor after '['");
    }

    auto index_expression = parse_standalone_expression(registry, expression_block.value());

    return std::make_unique<AstArrayMemberAccessor>(
        array_identifier->get_source(),
        array_identifier->get_source_position(),
        registry,
        std::move(array_identifier),
        std::move(index_expression)
    );
}

void AstArrayMemberAccessor::validate()
{
    const auto index_accessor_type = infer_expression_type(this->get_registry(), this->_index_accessor_expr.get());

    if (const auto primitive_type = dynamic_cast<AstPrimitiveType*>(index_accessor_type.get()))
    {
        if (!primitive_type->is_integer_ty())
        {
            throw parsing_error(
                ErrorType::SEMANTIC_ERROR,
                std::format("Array index accessor must be of get_type int, got '{}'", primitive_type->to_string()),
                *this->get_source(),
                this->get_source_position()
            );
        }
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        "Array index accessor must be of get_type 'int', got '" + index_accessor_type->to_string() + "'",
        *this->get_source(),
        this->get_source_position()
    );
}

llvm::Value* AstArrayMemberAccessor::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    const auto array_iden_type = infer_expression_type(this->get_registry(), this->_array_identifier.get());

    llvm::Value* base_ptr = this->_array_identifier->codegen(registry, module, builder);
    llvm::Value* index_val = this->_index_accessor_expr->codegen(registry, module, builder);

    // Element get_type, not the array get_type.
    // Assumes `array_iden_type` is something like "T[]" and has an element get_type you can extract.
    const auto* array_ty = dynamic_cast<AstArrayType*>(array_iden_type.get());
    if (!array_ty)
    {
        throw parsing_error(
                ErrorType::SEMANTIC_ERROR,
                "Array member accessor used on non-array get_type",
                *this->get_source(),
                this->get_source_position()
        );
    }

    llvm::Type* elem_llvm_ty = internal_type_to_llvm_type(array_ty->get_element_type(), module);

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

std::string AstArrayMemberAccessor::to_string()
{
    return std::format("ArrayAccess({}, {})", this->_array_identifier->to_string(),
                       this->_index_accessor_expr->to_string());
}

bool AstArrayMemberAccessor::is_reducible()
{
    // If the value is a literal, it's reducible for sure.
    if (dynamic_cast<AstLiteral*>(this->_array_identifier.get()))
    {
        return true;
    }

    // Otherwise, we'll perhaps be able to reduce it if the index accessor expression itself
    // is reducible.
    return this->_index_accessor_expr->is_reducible();
}

IAstNode* AstArrayMemberAccessor::reduce()
{
    return this;
}
