#pragma once
#include <memory>

namespace stride::ast
{
    class AstArray;
    class AstBinaryArithmeticOp;
    class IAstExpression;
    class AstFunctionCall;
    class AstLiteral;
    class AstMemberAccessor;
    class AstStructInitializer;
    class AstUnaryOp;
    class IAstType;

    /// Will attempt to resolve the provided expression into an IAstInternalFieldType
    std::unique_ptr<IAstType> infer_expression_type(IAstExpression* expr, int recursion_guard = 0);

    /// Infers the element type of an array expression
    std::unique_ptr<IAstType> infer_array_member_type(const AstArray* array);

    /// Infers the result type of a unary operation
    std::unique_ptr<IAstType> infer_unary_op_type(const AstUnaryOp* operation);

    /// Infers the result type of a binary arithmetic operation
    std::unique_ptr<IAstType> infer_binary_arithmetic_op_type(
        const AstBinaryArithmeticOp* operation);

    /// Infers the type of a literal expression
    std::unique_ptr<IAstType> infer_expression_literal_type(AstLiteral* literal);

    /// Infers the return type of a function call expression
    std::unique_ptr<IAstType> infer_function_call_return_type(const AstFunctionCall* fn_call);

    /// Infers the type produced by a struct initializer expression
    std::unique_ptr<IAstType> infer_struct_initializer_type(const AstStructInitializer* initializer);

    /// Infers the type of the field accessed via a member accessor expression
    std::unique_ptr<IAstType> infer_member_accessor_type(
        const AstMemberAccessor* member_accessor_expr);
}
