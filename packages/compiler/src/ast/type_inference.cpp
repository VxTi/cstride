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
            str->get_source(), str->get_source_position(), scope, PrimitiveType::STRING, 1
        );
    }

    if (const auto* fp_lit = dynamic_cast<AstFpLiteral*>(literal))
    {
        auto type = fp_lit->bit_count() > 32 ? PrimitiveType::FLOAT64 : PrimitiveType::FLOAT32;
        return std::make_unique<AstPrimitiveFieldType>(
            fp_lit->get_source(), fp_lit->get_source_position(), scope, type, fp_lit->bit_count() / BITS_PER_BYTE
        );
    }

    if (const auto* int_lit = dynamic_cast<AstIntLiteral*>(literal))
    {
        auto type = int_lit->is_signed()
                        ? (int_lit->bit_count() > 32 ? PrimitiveType::INT64 : PrimitiveType::INT32)
                        : (int_lit->bit_count() > 32 ? PrimitiveType::UINT64 : PrimitiveType::UINT32);

        return std::make_unique<AstPrimitiveFieldType>(
            int_lit->get_source(),
            int_lit->get_source_position(),
            scope,
            type,
            int_lit->bit_count() / BITS_PER_BYTE,
            int_lit->get_flags()
        );
    }

    if (const auto* char_lit = dynamic_cast<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            char_lit->get_source(), char_lit->get_source_position(), scope, PrimitiveType::CHAR, char_lit->bit_count()
        );
    }

    if (const auto* bool_lit = dynamic_cast<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveFieldType>(
            bool_lit->get_source(), bool_lit->get_source_position(), scope, PrimitiveType::BOOL, bool_lit->bit_count()
        );
    }

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        "Unable to resolve expression literal type",
        *literal->get_source(),
        literal->get_source_position()
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
        ErrorType::TYPE_ERROR,
        std::format(
            "Unable to resolve function invocation return type for function '{}'",
            fn_call->get_function_name()
        ),
        *fn_call->get_source(),
        fn_call->get_source_position()
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

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_unary_op_type(
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
                prim->get_source(),
                prim->get_source_position(),
                scope,
                prim->type(),
                prim->byte_size(),
                flags
            );
        }
        if (const auto* named = dynamic_cast<AstStructType*>(type.get()))
        {
            return std::make_unique<AstStructType>(
                named->get_source(),
                named->get_source_position(),
                scope,
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
                ErrorType::TYPE_ERROR,
                "Cannot dereference non-pointer type",
                *operation->get_source(),
                operation->get_source_position()
            );
        }

        const auto flags = type->get_flags() & ~SRFLAG_TYPE_PTR;

        if (const auto* prim = dynamic_cast<AstPrimitiveFieldType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveFieldType>(
                prim->get_source(),
                prim->get_source_position(),
                scope,
                prim->type(),
                prim->byte_size(),
                flags
            );
        }
        if (const auto* named = dynamic_cast<AstStructType*>(type.get()))
        {
            return std::make_unique<AstStructType>(
                named->get_source(),
                named->get_source_position(),
                scope,
                named->name(),
                flags
            );
        }
    }
    else if (op_type == UnaryOpType::LOGICAL_NOT)
    {
        return std::make_unique<AstPrimitiveFieldType>(
            operation->get_source(),
            operation->get_source_position(),
            scope,
            PrimitiveType::BOOL,
            1
        );
    }

    return type;
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_array_member_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const AstArray* array
)
{
    if (array->get_elements().empty())
    {
        // This is one of those cases where it's impossible to deduce the type
        // Therefore, we have an UNKNOWN type.
        return std::make_unique<AstPrimitiveFieldType>(
            array->get_source(),
            array->get_source_position(),
            scope,
            PrimitiveType::UNKNOWN,
            8, // Always a pointer
            0
        );
    }

    return infer_expression_type(scope, array->get_elements().front().get());
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_member_accessor_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const AstMemberAccessor* member_accessor
)
{
    const auto base_iden = dynamic_cast<AstIdentifier*>(member_accessor->get_base());
    if (!base_iden)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            *member_accessor->get_source(),
            member_accessor->get_source_position()
        );
    }

    const auto field_definition = member_accessor->get_registry()->field_lookup(base_iden->get_internal_name());

    if (!field_definition)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Member access base '{}' was not found", base_iden->get_name()),
            *member_accessor->get_source(),
            member_accessor->get_source_position()
        );
    }

    const auto struct_type = dynamic_cast<AstStructType*>(field_definition->get_type());

    if (!struct_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Member access base '{}' is not a struct", base_iden->get_name()),
            *member_accessor->get_source(),
            member_accessor->get_source_position()
        );
    }

    auto prev_struct_definition = member_accessor->get_registry()->get_struct_def(struct_type->get_internal_name());

    // We'll iterate over all middle accessors. If there's just a single accessor,
    // it gets skipped here.
    for (int i = 0; i < member_accessor->get_members().size() - 1; i++)
    {
        // Member is in between a chain (<...>.<sub1>.<sub2>.<...>),
        const auto segment_identifier = dynamic_cast<AstIdentifier*>(member_accessor->get_members()[i]);

        if (!segment_identifier)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                "Member access submember must be an identifier",
                *member_accessor->get_source(),
                member_accessor->get_source_position()
            );
        }

        // The type of the struct member is the name of the referring struct
        const auto segment_identifier_field_type = prev_struct_definition->get_field(segment_identifier->get_name());
        prev_struct_definition = member_accessor->get_registry()->get_struct_def(segment_identifier_field_type->get_internal_name());

        // If `prev_struct_definition` is `nullptr` here, we already know it's not a struct type,
        // as it's not registered
        if (!prev_struct_definition)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Struct member '{}' accessor does not exist", base_iden->get_name()),
                *member_accessor->get_source(),
                member_accessor->get_source_position()
            );
        }
    }

    const auto last_segment_identifier = dynamic_cast<AstIdentifier*>(member_accessor->get_members().back());
    if (!last_segment_identifier)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access submember must be an identifier",
            *member_accessor->get_source(),
            member_accessor->get_source_position()
        );
    }

    const auto last_segment_field_type = prev_struct_definition->get_field(last_segment_identifier->get_name());
    if (!last_segment_field_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Struct member '{}' accessor does not exist", last_segment_identifier->get_name()),
            *member_accessor->get_source(),
            member_accessor->get_source_position()
        );
    }

    return std::unique_ptr(last_segment_field_type->clone());
}

std::unique_ptr<IAstInternalFieldType> stride::ast::infer_struct_initializer_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const AstStructInitializer* initializer
)
{
    return std::make_unique<AstStructType>(
        initializer->get_source(),
        initializer->get_source_position(),
        scope,
        initializer->get_struct_name()
    );
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
                ErrorType::SEMANTIC_ERROR,
                std::format("Variable '{}' not found in scope", identifier->get_name()),
                *identifier->get_source(),
                identifier->get_source_position()
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
        return infer_unary_op_type(scope, operation);
    }

    if (dynamic_cast<AstLogicalOp*>(expr) || dynamic_cast<AstComparisonOp*>(expr))
    {
        // TODO: Do some validation on lhs and rhs; cannot compare strings with one another (yet)
        return std::make_unique<AstPrimitiveFieldType>(
            expr->get_source(),
            expr->get_source_position(),
            scope,
            PrimitiveType::BOOL,
            1,
            0
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

    if (const auto* array_expr = dynamic_cast<AstArray*>(expr))
    {
        auto member_type = infer_array_member_type(scope, array_expr);

        return std::make_unique<AstArrayType>(
            array_expr->get_source(),
            array_expr->get_source_position(),
            scope,
            std::move(member_type),
            array_expr->get_elements().size()
        );
    }

    if (const auto* array_accessor = dynamic_cast<AstArrayMemberAccessor*>(expr))
    {
        const auto array_type = infer_expression_type(scope, array_accessor->get_array_identifier());

        if (const auto array = dynamic_cast<AstArrayType*>(array_type.get()); array != nullptr)
        {
            return array->get_element_type()->clone();
        }
    }

    if (const auto* struct_init = dynamic_cast<AstStructInitializer*>(expr))
    {
        return infer_struct_initializer_type(scope, struct_init);
    }

    if (const auto* member_accessor = dynamic_cast<AstMemberAccessor*>(expr))
    {
        return infer_member_accessor_type(scope, member_accessor);
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        "Unable to resolve expression type",
        *expr->get_source(),
        expr->get_source_position()
    );
}
