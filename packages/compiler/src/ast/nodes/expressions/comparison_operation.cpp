#include "errors.h"
#include "ast/casting.h"
#include "ast/optionals.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;

std::optional<ComparisonOpType> stride::ast::get_comparative_op_type(
    const TokenType type)
{
    switch (type)
    {
    case TokenType::DOUBLE_EQUALS:
        return ComparisonOpType::EQUALS;
    case TokenType::BANG_EQUALS:
        return ComparisonOpType::NOT_EQUAL;
    case TokenType::LT:
        return ComparisonOpType::LESS_THAN;
    case TokenType::LEQUALS:
        return ComparisonOpType::LESS_THAN_OR_EQUAL;
    case TokenType::GT:
        return ComparisonOpType::GREATER_THAN;
    case TokenType::GEQUALS:
        return ComparisonOpType::GREATER_THAN_OR_EQUAL;
    default:
        return std::nullopt;
    }
}

std::string comparison_op_to_str(const ComparisonOpType op)
{
    switch (op)
    {
    case ComparisonOpType::EQUALS:
        return "==";
    case ComparisonOpType::NOT_EQUAL:
        return "!=";
    case ComparisonOpType::LESS_THAN:
        return "<";
    case ComparisonOpType::LESS_THAN_OR_EQUAL:
        return "<=";
    case ComparisonOpType::GREATER_THAN:
        return ">";
    case ComparisonOpType::GREATER_THAN_OR_EQUAL:
        return ">=";
    default:
        return "";
    }
}

void AstComparisonOp::validate()
{
    this->_lhs->validate();
    this->_rhs->validate();

    const auto lhs_type = this->get_left()->get_type();
    const auto rhs_type = this->get_right()->get_type();

    // Both sides are primitives
    if (lhs_type->is_primitive() && rhs_type->is_primitive())
        return;

    const auto lhs_primitive = cast_type<AstPrimitiveType*>(lhs_type);
    const auto rhs_primitive = cast_type<AstPrimitiveType*>(rhs_type);

    // If LHS is NIL and RHS is valid, allow the comparison (nil checks)
    if (lhs_primitive && rhs_primitive
        && lhs_primitive->get_primitive_type() == PrimitiveType::NIL
        && rhs_primitive->get_primitive_type() != PrimitiveType::NIL)
        return;

    // visa versa; LHS is primitive and RHS is nil
    if (rhs_primitive && lhs_primitive
        && rhs_primitive->get_primitive_type() == PrimitiveType::NIL
        && lhs_primitive->get_primitive_type() != PrimitiveType::NIL)
        return;

    const auto lhs_struct = cast_type<AstNamedType*>(lhs_type);
    const auto rhs_struct = cast_type<AstNamedType*>(rhs_type);

    // LHS is optional struct and RHS is primitive and RHS is not nil
    if (lhs_struct && rhs_primitive && lhs_struct->is_optional() &&
        rhs_primitive->get_primitive_type() == PrimitiveType::NIL)
        return;

    if (lhs_primitive && rhs_struct && rhs_struct->is_optional() &&
        lhs_primitive->get_primitive_type() == PrimitiveType::NIL)
        return;

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        "Comparison operation operands must be used on primitive or optional types",
        this->get_source_fragment()
    );
}

llvm::Value* AstComparisonOp::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Value* left = this->get_left()->codegen(module, builder);
    llvm::Value* right = this->get_right()->codegen(module, builder);

    if (!left || !right)
    {
        return nullptr;
    }

    const auto lhs_optional_ty = is_optional_wrapped_type(left->getType());
    const auto rhs_optional_ty = is_optional_wrapped_type(right->getType());

    if (lhs_optional_ty || rhs_optional_ty)
    {
        llvm::Value* struct_val = nullptr;

        if (lhs_optional_ty && llvm::isa<llvm::ConstantPointerNull>(right))
        {
            struct_val = left;
        }
        else if (rhs_optional_ty && llvm::isa<llvm::ConstantPointerNull>(left))
        {
            struct_val = right;
        }

        if (!struct_val)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "Cannot compare a non-optional value with nil",
                this->get_source_fragment()
            );
        }

        llvm::Value* has_value = builder->CreateExtractValue(
            struct_val,
            { OPT_IDX_HAS_VALUE },
            "has_value"
        );

        if (this->get_op_type() == ComparisonOpType::NOT_EQUAL)
        {
            // val != nil  -> has_value == 1
            return builder->CreateICmpEQ(
                has_value,
                llvm::ConstantInt::get(
                    llvm::Type::getInt1Ty(module->getContext()),
                    OPT_HAS_VALUE),
                "not_nil_check"
            );
        }
        if (this->get_op_type() == ComparisonOpType::EQUALS)
        {
            // val == nil -> has_value == 0
            return builder->CreateICmpEQ(
                has_value,
                llvm::ConstantInt::get(
                    llvm::Type::getInt1Ty(module->getContext()),
                    OPT_NO_VALUE),
                "is_nil_check"
            );
        }
    }

    const auto left_ty = left->getType();
    const auto right_ty = right->getType();

    // Cast operands if they are integers and have different bit widths
    if (left_ty->isIntegerTy() && right_ty->isIntegerTy())
    {
        const auto left_width = left_ty->getIntegerBitWidth();

        if (const auto right_width = right_ty->getIntegerBitWidth();
            left_width < right_width)
        {
            left = builder->CreateIntCast(left, right_ty, true, "icmp_sext");
        }
        else if (right_width < left_width)
        {
            right = builder->CreateIntCast(right, left_ty, true, "icmp_sext");
        }
    }
    else
    {
        llvm::Type* target_type = builder->getDoubleTy();
        if (left_ty->isFloatTy() && right_ty->isFloatTy())
        {
            target_type = builder->getFloatTy();
        }

        // Cast Left
        if (left_ty->isIntegerTy())
        {
            left = builder->CreateSIToFP(left, target_type, "sitofp");
        }
        else
        {
            left = builder->CreateFPCast(left, target_type, "fpcast");
        }

        // Cast Right
        if (right_ty->isIntegerTy())
        {
            right = builder->CreateSIToFP(right, target_type, "sitofp");
        }
        else
        {
            right = builder->CreateFPCast(right, target_type, "fpcast");
        }
    }

    // Check if operands are floating point or integer
    const bool is_float =
        left_ty->isFloatingPointTy() ||
        right_ty->isFloatingPointTy();

    switch (this->get_op_type())
    {
    case ComparisonOpType::EQUALS:
        return is_float
            ? builder->CreateFCmpOEQ(left, right, "eqtmp")
            : builder->CreateICmpEQ(left, right, "eqtmp");
    case ComparisonOpType::NOT_EQUAL:
        return is_float
            ? builder->CreateFCmpONE(left, right, "netmp")
            : builder->CreateICmpNE(left, right, "netmp");
    case ComparisonOpType::LESS_THAN:
        return is_float
            ? builder->CreateFCmpOLT(left, right, "lttmp")
            : builder->CreateICmpSLT(left, right, "lttmp");
    case ComparisonOpType::LESS_THAN_OR_EQUAL:
        return is_float
            ? builder->CreateFCmpOLE(left, right, "letmp")
            : builder->CreateICmpSLE(left, right, "letmp");
    case ComparisonOpType::GREATER_THAN:
        return is_float
            ? builder->CreateFCmpOGT(left, right, "gttmp")
            : builder->CreateICmpSGT(left, right, "gttmp");
    case ComparisonOpType::GREATER_THAN_OR_EQUAL:
        return is_float
            ? builder->CreateFCmpOGE(left, right, "getmp")
            : builder->CreateICmpSGE(left, right, "getmp");
    default:
        return nullptr;
    }
}

std::unique_ptr<IAstNode> AstComparisonOp::clone()
{
    return std::make_unique<AstComparisonOp>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_left()->clone_as<IAstExpression>(),
        this->_op_type,
        this->get_right()->clone_as<IAstExpression>()
    );
}

std::string AstComparisonOp::to_string()
{
    return std::format(
        "ComparisonOp({}, {}, {})",
        this->get_left()->to_string(),
        comparison_op_to_str(this->_op_type),
        this->get_right()->to_string());
}
