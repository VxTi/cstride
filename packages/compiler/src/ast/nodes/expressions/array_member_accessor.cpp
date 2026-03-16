#include "errors.h"
#include "ast/casting.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

std::unique_ptr<AstArrayMemberAccessor> stride::ast::parse_array_member_accessor(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<IAstExpression> array_base)
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

    const auto source_pos = SourceFragment::join(array_base->get_source_fragment(), last_src_pos);

    return std::make_unique<AstArrayMemberAccessor>(
        source_pos,
        context,
        std::move(array_base),
        std::move(index_expression)
    );
}

void AstArrayMemberAccessor::validate()
{
    this->_array_base->validate();
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
    std::unique_ptr<IAstType> array_base_type = this->_array_base->get_type()->clone_ty();

    if (const auto named_ty = cast_type<AstAliasType*>(array_base_type.get()))
    {
        array_base_type = named_ty->get_underlying_type()->clone_ty();
    }

    llvm::Value* base_val = this->_array_base->codegen(module, builder);
    llvm::Value* index_val = this->_index_accessor_expr->codegen(module, builder);

    const auto* array_ty = cast_type<AstArrayType*>(array_base_type.get());
    if (!array_ty)
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            "Array member accessor used on non-array type",
            this->get_source_fragment());
    }

    llvm::Type* elem_llvm_ty = array_ty->get_element_type()->get_llvm_type(module);

    // Ensure we have a pointer to GEP into. The base expression's codegen may
    // have produced a non-pointer value (e.g. identifier codegen loads the
    // alloca's allocated type which can be [0 x T] for dynamically-sized arrays).
    // In that case, look through the load to recover the source pointer.
    llvm::Value* base_ptr = base_val;
    if (!base_ptr->getType()->isPointerTy())
    {
        if (auto* load_inst = llvm::dyn_cast<llvm::LoadInst>(base_val))
        {
            base_ptr = load_inst->getPointerOperand();
            load_inst->eraseFromParent();
        }
    }

    // If the base is a pointer to an alloca of array type, load the stored
    // pointer first (arrays are always behind a pointer in stride).
    if (base_ptr->getType()->isPointerTy())
    {
        if (const auto* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(base_ptr))
        {
            llvm::Type* allocated_ty = alloca_inst->getAllocatedType();
            if (allocated_ty->isArrayTy())
            {
                // The alloca holds a pointer to the array data. Load it.
                llvm::Value* array_ptr = builder->CreateLoad(
                    llvm::PointerType::getUnqual(module->getContext()),
                    base_ptr,
                    "array_ptr"
                );

                llvm::Value* element_ptr = builder->CreateInBoundsGEP(
                    elem_llvm_ty,
                    array_ptr,
                    index_val,
                    "array_elem_ptr"
                );

                return builder->CreateLoad(elem_llvm_ty, element_ptr, "array_load");
            }
        }
    }

    // Opaque pointer or decayed pointer-to-element: single-index GEP.
    llvm::Value* element_ptr = builder->CreateInBoundsGEP(
        elem_llvm_ty,
        base_ptr,
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
        this->_array_base->clone_as<IAstExpression>(),
        this->_index_accessor_expr->clone_as<IAstExpression>()
    );
}

std::string AstArrayMemberAccessor::to_string()
{
    return std::format(
        "ArrayAccess({}, {})",
        this->_array_base->to_string(),
        this->_index_accessor_expr->to_string()
    );
}

bool AstArrayMemberAccessor::is_reducible()
{
    // If the value is a literal, it's reducible for sure.
    if (cast_expr<AstLiteral*>(this->_array_base.get()))
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
