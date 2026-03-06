#include "ast/nodes/expression.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::optional<std::unique_ptr<IAstExpression>> stride::ast::parse_type_cast_op(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    IAstExpression* lhs
)
{
    if (!set.peek_next_eq(TokenType::KEYWORD_AS))
        return std::nullopt;

    set.next();

    auto type = parse_type(context, set, "Expected type after 'as' in type cast operation");

    const auto source_fragment = SourceFragment::concat(lhs->get_source_fragment(), type->get_source_fragment());

    return std::make_unique<AstTypeCastOp>(
        source_fragment,
        context,
        lhs->clone_as<IAstExpression>(),
        std::move(type)
    );
}

void AstTypeCastOp::validate()
{
    if (!this->_value)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Type cast operation is missing a value to cast",
            this->get_source_fragment()
        );
    }

    if (!this->_target_type->is_assignable_to(this->_value->get_type()))
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Cannot cast value of type '{}' to incompatible type '{}'",
                this->_value->get_type()->to_string(),
                this->_target_type->to_string()
            ),
            this->get_source_fragment()
        );
    }
}

llvm::Value* AstTypeCastOp::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    const auto value = this->_value->codegen(module, builder);
    if (this->_value->get_type()->equals(*this->_target_type))
    {
        // No cast needed, return the original value.
        return value;
    }

    const auto value_ty = value->getType();
    const auto target_ty = type_to_llvm_type(this->_target_type.get(), module);

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

    return builder->CreateBitCast(value, target_ty);
}

std::unique_ptr<IAstNode> AstTypeCastOp::clone()
{
    return std::make_unique<AstTypeCastOp>(
        this->get_source_fragment(),
        this->get_context(),
        this->_value->clone_as<IAstExpression>(),
        this->_target_type->clone_ty()
    );
}

std::string AstTypeCastOp::to_string()
{
    return std::format("TypeCastOp({}, {})", this->_value->to_string(), this->_target_type->to_string());
}
