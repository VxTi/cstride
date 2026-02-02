#include <llvm/IR/Module.h>

#include "ast/optionals.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

std::optional<ComparisonOpType> stride::ast::get_comparative_op_type(const TokenType type)
{
    switch (type)
    {
    case TokenType::DOUBLE_EQUALS: return ComparisonOpType::EQUAL;
    case TokenType::BANG_EQUALS: return ComparisonOpType::NOT_EQUAL;
    case TokenType::LARROW: return ComparisonOpType::LESS_THAN;
    case TokenType::LEQUALS: return ComparisonOpType::LESS_THAN_OR_EQUAL;
    case TokenType::RARROW: return ComparisonOpType::GREATER_THAN;
    case TokenType::GEQUALS: return ComparisonOpType::GREATER_THAN_OR_EQUAL;
    default: return std::nullopt;
    }
}

std::string comparison_op_to_str(const ComparisonOpType op)
{
    switch (op)
    {
    case ComparisonOpType::EQUAL: return "==";
    case ComparisonOpType::NOT_EQUAL: return "!=";
    case ComparisonOpType::LESS_THAN: return "<";
    case ComparisonOpType::LESS_THAN_OR_EQUAL: return "<=";
    case ComparisonOpType::GREATER_THAN: return ">";
    case ComparisonOpType::GREATER_THAN_OR_EQUAL: return ">=";
    default: return "";
    }
}

std::string AstComparisonOp::to_string()
{
    return std::format(
        "ComparisonOp({}, {}, {})",
        this->get_left()->to_string(), comparison_op_to_str(this->_op_type), this->get_right()->to_string()
    );
}

llvm::Value* AstComparisonOp::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    llvm::Value* left = this->get_left()->codegen(registry, module, builder);
    llvm::Value* right = this->get_right()->codegen(registry, module, builder);

    if (!left || !right)
    {
        return nullptr;
    }

    const auto lhs_optional_ty =  is_optional_wrapped_type(left->getType());
    const auto rhs_optional_ty =  is_optional_wrapped_type(right->getType());

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
                ErrorType::RUNTIME_ERROR,
                "Cannot compare a non-optional value with nil",
                *this->get_source(),
                this->get_source_position()
            );
        }

        llvm::Value* has_value = builder->CreateExtractValue(struct_val, {OPT_IDX_HAS_VALUE}, "has_value");

        if (this->get_op_type() == ComparisonOpType::NOT_EQUAL)
        {
            // val != nil  -> has_value == 1
            return builder->CreateICmpEQ(
                has_value,
                llvm::ConstantInt::get(
                    llvm::Type::getInt1Ty(module->getContext()),
                    OPT_HAS_VALUE
                ),
                "not_nil_check"
            );
        }
        if (this->get_op_type() == ComparisonOpType::EQUAL)
        {
            // val == nil -> has_value == 0
            return builder->CreateICmpEQ(
                has_value,
                llvm::ConstantInt::get(
                    llvm::Type::getInt1Ty(module->getContext()),
                    OPT_NO_VALUE
                ),
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
    const bool is_float = left_ty->isFloatingPointTy() || right_ty->isFloatingPointTy();

    switch (this->get_op_type())
    {
    case ComparisonOpType::EQUAL:
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
