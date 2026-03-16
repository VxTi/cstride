#pragma once
#include <memory>

namespace stride::ast
{
    class AstArrayMemberAccessor;
    class AstVariableDeclaration;
    class AstIdentifier;
    class AstArray;
    class IBinaryOp;
    class IAstExpression;
    class AstFunctionCall;
    class AstLiteral;
    class AstChainedExpression;
    class AstIndirectCall;
    class AstObjectInitializer;
    class AstUnaryOp;
    class IAstType;
    class IAstFunction;

    /// Will attempt to resolve the provided expression into an IAstInternalFieldType
    std::unique_ptr<IAstType> infer_expression_type(IAstExpression* expr, int recursion_guard = 0);

    /// Infers the element type of an array expression
    std::unique_ptr<IAstType> infer_array_member_type(const AstArray* array);

    /// Infers the result type of a unary operation
    std::unique_ptr<IAstType> infer_unary_op_type(const AstUnaryOp* operation);

    /// Infers the result type of a binary operation
    std::unique_ptr<IAstType> infer_binary_op_type(IBinaryOp* operation);

    /// Infers the type of a literal expression
    std::unique_ptr<IAstType> infer_expression_literal_type(const AstLiteral* literal);

    /// Infers the return type of a function call expression
    std::unique_ptr<IAstType> infer_function_call_return_type(AstFunctionCall* fn_call);

    /// Infers the type produced by a struct initializer expression
    std::unique_ptr<IAstType> infer_object_initializer_type(const AstObjectInitializer* struct_initializer);

    /// Infers the type of an identifier by looking it up in the current context
    std::unique_ptr<IAstType> infer_identifier_type(const AstIdentifier* identifier);

    /// Infers the type of a variable declaration, which is either the explicitly declared type or the type of the initializer expression
    std::unique_ptr<IAstType> infer_variable_declaration_type(
        const AstVariableDeclaration* declaration,
        int recursion_guard);

    /// Infers the type produced by a chained expression (base.followup member access)
    std::unique_ptr<IAstType> infer_chained_expression_type(const AstChainedExpression* chained_expr);

    /// Infers the return type of an indirect call expression (expr(args))
    std::unique_ptr<IAstType> infer_indirect_call_type(const AstIndirectCall* call_expr);

    /// Infers the element type produced by an array subscript expression
    std::unique_ptr<IAstType> infer_array_accessor_type(const AstArrayMemberAccessor* accessor, int recursion_guard);

    std::unique_ptr<IAstType> infer_function_type(
        const IAstFunction* function);
}
