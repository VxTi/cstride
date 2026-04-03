#include "ast/nodes/expression.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::optional<std::unique_ptr<IAstExpression>> stride::ast::parse_type_cast_op(
    TokenSet& set,
    IAstExpression* lhs
)
{
    if (!set.peek_next_eq(TokenType::KEYWORD_AS))
        return std::nullopt;

    set.next();

    auto type = parse_type(set, { "Expected type after 'as' in type cast operation" });

    const auto source_fragment = SourcePosition::join(lhs->get_source_position(), type->get_source_position());

    return std::make_unique<AstTypeCastOp>(
        source_fragment,
        lhs->clone_as<IAstExpression>(),
        std::move(type)
    );
}

void AstTypeCastOp::validate(SymbolTable* symbol_table)
{
    if (!this->_value)
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            "Type cast operation is missing a value to cast",
            this->get_source_position()
        );
    }

    if (!this->_value->get_type()->is_castable_to(this->_target_type.get()))
    {
        throw stride_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Cannot cast value of type '{}' to incompatible type '{}'",
                this->_value->get_type()->get_type_name(),
                this->_target_type->get_type_name()
            ),
            this->get_source_position()
        );
    }
}

llvm::Value* AstTypeCastOp::codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder)
{
    const auto value = this->_value->codegen(symbol_table, module, builder);

    if (this->_value->get_type()->equals(this->_target_type.get()))
    {
        // No cast needed, return the original value.
        return value;
    }

    const auto value_ty = value->getType();
    const auto target_ty = this->_target_type->get_llvm_type(module);

    if (value_ty->isIntegerTy() && target_ty->isIntegerTy())
    {
        const auto value_width = value_ty->getIntegerBitWidth();
        const auto target_width = target_ty->getIntegerBitWidth();

        if (value_width < target_width)
        {
            return builder->CreateIntCast(value, target_ty, true);
        }

        if (value_width > target_width)
        {
            return builder->CreateIntCast(value, target_ty, false);
        }

        return value;
    }

    if (value_ty->isFloatingPointTy() && target_ty->isFloatingPointTy())
    {
        const auto value_width = value_ty->getPrimitiveSizeInBits();
        const auto target_width = target_ty->getPrimitiveSizeInBits();

        if (value_width < target_width)
        {
            return builder->CreateFPExt(value, target_ty);
        }

        if (value_width > target_width)
        {
            return builder->CreateFPTrunc(value, target_ty);
        }

        return value;
    }

    if (value_ty->isIntegerTy() && target_ty->isFloatingPointTy())
    {
        return builder->CreateSIToFP(value, target_ty);
    }

    if (value_ty->isFloatingPointTy() && target_ty->isIntegerTy())
    {
        return builder->CreateFPToSI(value, target_ty);
    }

    return builder->CreateBitCast(value, target_ty);
}

std::unique_ptr<IAstNode> AstTypeCastOp::clone()
{
    return std::make_unique<AstTypeCastOp>(
        this->get_source_position(),
        this->_value->clone_as<IAstExpression>(),
        this->_target_type->clone(),
        this->clone_type()
    );
}

std::string AstTypeCastOp::to_string()
{
    return std::format("TypeCastOp({}, {})", this->_value->to_string(), this->_target_type->get_type_name());
}
