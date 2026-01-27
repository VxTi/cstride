#include <memory>

#include "ast/flags.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_expression_literal_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    AstLiteral* literal
)
{
    if (const auto* str = dynamic_cast<AstStringLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            str->source, str->source_offset, scope, PrimitiveType::STRING, 1);
    }

    if (const auto* fp_lit = dynamic_cast<AstFpLiteral*>(literal))
    {
        auto type = fp_lit->bit_count() > 32 ? PrimitiveType::FLOAT64 : PrimitiveType::FLOAT32;
        return std::make_unique<AstPrimitiveFieldType>(
            fp_lit->source, fp_lit->source_offset, scope, type, fp_lit->bit_count() / BITS_PER_BYTE
        );
    }

    if (const auto* int_lit = dynamic_cast<AstIntLiteral*>(literal))
    {
        auto type = int_lit->is_signed()
                        ? (int_lit->bit_count() > 32 ? PrimitiveType::INT64 : PrimitiveType::INT32)
                        : (int_lit->bit_count() > 32 ? PrimitiveType::UINT64 : PrimitiveType::UINT32);

        return std::make_unique<AstPrimitiveFieldType>(
            int_lit->source,
            int_lit->source_offset,
            scope,
            type,
            int_lit->bit_count() / BITS_PER_BYTE,
            int_lit->get_flags()
        );
    }

    if (const auto* char_lit = dynamic_cast<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            char_lit->source, char_lit->source_offset, scope, PrimitiveType::CHAR, char_lit->bit_count()
        );
    }

    if (const auto* bool_lit = dynamic_cast<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            bool_lit->source, bool_lit->source_offset, scope, PrimitiveType::BOOL, bool_lit->bit_count()
        );
    }

    throw std::runtime_error(
        make_ast_error(*literal->source, literal->source_offset, "Unable to resolve expression literal type")
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_function_call_return_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const AstFunctionCall* fn_call
)
{
    if (const auto fn_def = scope->get_function_def(fn_call->get_internal_name());
        fn_def != nullptr)
    {
        return fn_def->get_return_type()->clone();
    }

    // It could be an extern function, in which case the function name is just as-is
    if (const auto fn_def = scope->get_function_def(fn_call->get_function_name());
        fn_def != nullptr)
    {
        return fn_def->get_return_type()->clone();
    }

    throw parsing_error(
        make_ast_error(
            *fn_call->source,
            fn_call->source_offset,
            std::format(
                "Unable to resolve function invocation return type for function '{}'",
                fn_call->get_function_name()
            )
        )
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_binary_arithmetic_op_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const AstBinaryArithmeticOp* operation
)
{
    auto lhs = infer_expression_type(scope, &operation->get_left());
    auto rhs = infer_expression_type(scope, &operation->get_right());

    if (*lhs == *rhs)
    {
        return std::move(lhs);
    }

    if (lhs->is_pointer() && !rhs->is_pointer())
    {
        return std::move(lhs);
    }

    if (!lhs->is_pointer() && rhs->is_pointer())
    {
        return std::move(rhs);
    }

    return get_dominant_field_type(scope, lhs.get(), rhs.get());
}

std::unique_ptr<IAstInternalFieldType> stride::ast::resolve_unary_op_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const AstUnaryOp* operation
)
{
    auto type = infer_expression_type(scope, &operation->get_operand());

    if (const auto op_type = operation->get_op_type(); op_type == UnaryOpType::ADDRESS_OF)
    {
        const auto flags = type->get_flags() | SRFLAG_TYPE_PTR;
        if (const auto* prim = dynamic_cast<AstPrimitiveFieldType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveFieldType>(
                prim->source, prim->source_offset, scope,
                prim->type(), prim->byte_size(),
                flags
            );
        }
        if (const auto* named = dynamic_cast<AstNamedValueType*>(type.get()))
        {
            return std::make_unique<AstNamedValueType>(
                named->source, named->source_offset, scope,
                named->name(),
                flags
            );
        }
    }
    else if (op_type == UnaryOpType::DEREFERENCE)
    {
        if (!type->is_pointer())
        {
            throw parsing_error(
                make_ast_error(*operation->source, operation->source_offset, "Cannot dereference non-pointer type")
            );
        }

        const auto flags = type->get_flags() & ~SRFLAG_TYPE_PTR;

        if (const auto* prim = dynamic_cast<AstPrimitiveFieldType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveFieldType>(
                prim->source, prim->source_offset, scope,
                prim->type(), prim->byte_size(),
                flags
            );
        }
        if (const auto* named = dynamic_cast<AstNamedValueType*>(type.get()))
        {
            return std::make_unique<AstNamedValueType>(
                named->source, named->source_offset, scope,
                named->name(),
                flags
            );
        }
    }
    else if (op_type == UnaryOpType::LOGICAL_NOT)
    {
        return std::make_unique<AstPrimitiveFieldType>(
            operation->source, operation->source_offset, scope, PrimitiveType::BOOL, 1
        );
    }

    return type;
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_array_member_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    AstArray* array
)
{
    if (array->get_elements().empty())
    {
        // This is one of those cases where it's impossible to deduce the type
        // Therefore, we have an UNKNOWN type.
        return std::make_unique<AstPrimitiveFieldType>(
            array->source,
            array->source_offset,
            scope,
            PrimitiveType::UNKNOWN,
            8, // Always a pointer
            0
        );
    }

    return infer_expression_type(scope, array->get_elements().front().get());
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_expression_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    AstExpression* expr
)
{
    if (auto* literal = dynamic_cast<AstLiteral*>(expr))
    {
        return infer_expression_literal_type(scope, literal);
    }

    if (const auto* identifier = dynamic_cast<AstIdentifier*>(expr))
    {
        // TODO: Add generic support.
        // Right now we just do a lookup for `identifier`'s name, though we might want to extend
        // the lookup for generics.
        const auto reference_variable = scope->field_lookup(identifier->get_name());
        if (!reference_variable)
        {
            throw parsing_error(
                make_ast_error(
                    *identifier->source, identifier->source_offset,
                    std::format("Variable '{}' not found in scope", identifier->get_name())
                )
            );
        }

        return reference_variable->get_type()->clone();
    }

    if (const auto* operation = dynamic_cast<AstBinaryArithmeticOp*>(expr))
    {
        return infer_binary_arithmetic_op_type(scope, operation);
    }

    if (const auto* operation = dynamic_cast<AstUnaryOp*>(expr))
    {
        return resolve_unary_op_type(scope, operation);
    }

    if (dynamic_cast<AstLogicalOp*>(expr) || dynamic_cast<AstComparisonOp*>(expr))
    {
        // TODO: Do some validation on lhs and rhs; cannot compare strings with one another (yet)
        return std::make_unique<AstPrimitiveFieldType>(
            expr->source, expr->source_offset, scope, PrimitiveType::BOOL, 1, 0
        );
    }

    if (const auto* operation = dynamic_cast<AstVariableReassignment*>(expr))
    {
        return infer_expression_type(scope, operation->get_value());
    }

    if (const auto* operation = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        const auto lhs_variable_type = operation->get_variable_type();
        const auto value = infer_expression_type(scope, operation->get_initial_value().get());

        // Both the expression type and the declared type are the same (e.g., let var: i32 = 10)
        // so we can just return the declared type.
        if (*lhs_variable_type == *value)
        {
            return lhs_variable_type->clone();
        }

        return get_dominant_field_type(scope, lhs_variable_type, value.get());
    }

    if (const auto* fn_call = dynamic_cast<AstFunctionCall*>(expr))
    {
        return infer_function_call_return_type(scope, fn_call);
    }

    if (auto* array_expr = dynamic_cast<AstArray*>(expr))
    {
        auto member_type = infer_array_member_type(scope, array_expr);

        return std::make_unique<AstArrayType>(
            array_expr->source,
            array_expr->source_offset,
            scope,
            std::move(member_type),
            array_expr->get_elements().size()
        );
    }

    throw parsing_error(
        make_source_error(
            *expr->source,
            ErrorType::SEMANTIC_ERROR,
            "Unable to resolve expression type",
            expr->source_offset,
            expr->to_string().size()
        )
    );
}
