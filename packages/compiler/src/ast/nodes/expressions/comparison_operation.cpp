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
        this->get_left().to_string(), comparison_op_to_str(this->_op_type), this->get_right().to_string()
    );
}

llvm::Value* AstComparisonOp::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    llvm::Value* left = this->get_left().codegen(scope, module, context, builder);
    llvm::Value* right = this->get_right().codegen(scope, module, context, builder);

    if (!left || !right)
    {
        return nullptr;
    }

    // Cast operands if they are integers and have different bit widths
    if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy())
    {
        const auto left_width = left->getType()->getIntegerBitWidth();
        const auto right_width = right->getType()->getIntegerBitWidth();

        if (left_width < right_width)
        {
            left = builder->CreateIntCast(left, right->getType(), true, "icmp_sext");
        }
        else if (right_width < left_width)
        {
            right = builder->CreateIntCast(right, left->getType(), true, "icmp_sext");
        }
    } else
    {
        llvm::Type* target_type = builder->getDoubleTy();
        if (left->getType()->isFloatTy() && right->getType()->isFloatTy()) {
            target_type = builder->getFloatTy();
        }

        // 2. Cast Left
        if (left->getType()->isIntegerTy()) {
            left = builder->CreateSIToFP(left, target_type, "sitofp");
        } else {
            left = builder->CreateFPCast(left, target_type, "fpcast");
        }

        // 3. Cast Right
        if (right->getType()->isIntegerTy()) {
            right = builder->CreateSIToFP(right, target_type, "sitofp");
        } else {
            right = builder->CreateFPCast(right, target_type, "fpcast");
        }
    }

    // Check if operands are floating point or integer
    const bool is_float = left->getType()->isFloatingPointTy() || right->getType()->isFloatingPointTy();

    switch (this->_op_type)
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
