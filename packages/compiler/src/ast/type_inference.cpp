#include <memory>

#include "ast/flags.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

std::unique_ptr<IAstType> stride::ast::infer_expression_literal_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    AstLiteral* literal
)
{
    if (const auto* str = dynamic_cast<AstStringLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(
            str->get_source(),
            str->get_source_position(),
            registry,
            PrimitiveType::STRING,
            1
        );
    }

    if (const auto* fp_lit = dynamic_cast<AstFpLiteral*>(literal))
    {
        auto type = fp_lit->bit_count() > 32 ? PrimitiveType::FLOAT64 : PrimitiveType::FLOAT32;
        return std::make_unique<AstPrimitiveType>(
            fp_lit->get_source(),
            fp_lit->get_source_position(),
            registry,
            type,
            fp_lit->bit_count()
        );
    }

    if (const auto* int_lit = dynamic_cast<AstIntLiteral*>(literal))
    {
        auto type = int_lit->is_signed()
                        ? (int_lit->bit_count() > 32 ? PrimitiveType::INT64 : PrimitiveType::INT32)
                        : (int_lit->bit_count() > 32 ? PrimitiveType::UINT64 : PrimitiveType::UINT32);

        return std::make_unique<AstPrimitiveType>(
            int_lit->get_source(),
            int_lit->get_source_position(),
            registry,
            type,
            int_lit->bit_count(),
            int_lit->get_flags()
        );
    }

    if (const auto* char_lit = dynamic_cast<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(
            char_lit->get_source(),
            char_lit->get_source_position(),
            registry,
            PrimitiveType::CHAR,
            char_lit->bit_count()
        );
    }

    if (const auto* bool_lit = dynamic_cast<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(
            bool_lit->get_source(),
            bool_lit->get_source_position(),
            registry,
            PrimitiveType::BOOL,
            bool_lit->bit_count()
        );
    }

    if (const auto* nil_lit = dynamic_cast<AstNilLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(
            nil_lit->get_source(),
            nil_lit->get_source_position(),
            registry,
            PrimitiveType::NIL,
            8
        );
    }

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        "Unable to resolve expression literal type",
        *literal->get_source(),
        literal->get_source_position()
    );
}

std::unique_ptr<IAstType> stride::ast::infer_function_call_return_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    const AstFunctionCall* fn_call
)
{
    if (const auto fn_def = registry->get_function_def(fn_call->get_internal_name());
        fn_def != nullptr)
    {
        return fn_def->get_return_type()->clone();
    }

    // It could be an extern function, in which case the function name is just as-is
    if (const auto fn_def = registry->get_function_def(fn_call->get_function_name());
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

std::unique_ptr<IAstType> stride::ast::infer_binary_arithmetic_op_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    const AstBinaryArithmeticOp* operation
)
{
    auto lhs = infer_expression_type(registry, &operation->get_left());
    auto rhs = infer_expression_type(registry, &operation->get_right());

    if (lhs->equals(*rhs))
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

    return get_dominant_field_type(registry, lhs.get(), rhs.get());
}

std::unique_ptr<IAstType> stride::ast::infer_unary_op_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    const AstUnaryOp* operation
)
{
    auto type = infer_expression_type(registry, &operation->get_operand());

    if (const auto op_type = operation->get_op_type(); op_type == UnaryOpType::ADDRESS_OF)
    {
        const auto flags = type->get_flags() | SRFLAG_TYPE_PTR;
        if (const auto* prim = dynamic_cast<AstPrimitiveType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveType>(
                prim->get_source(),
                prim->get_source_position(),
                registry,
                prim->get_type(),
                prim->bit_count(),
                flags
            );
        }
        if (const auto* named = dynamic_cast<AstStructType*>(type.get()))
        {
            return std::make_unique<AstStructType>(
                named->get_source(),
                named->get_source_position(),
                registry,
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

        if (const auto* prim = dynamic_cast<AstPrimitiveType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveType>(
                prim->get_source(),
                prim->get_source_position(),
                registry,
                prim->get_type(),
                prim->bit_count(),
                flags
            );
        }
        if (const auto* named = dynamic_cast<AstStructType*>(type.get()))
        {
            return std::make_unique<AstStructType>(
                named->get_source(),
                named->get_source_position(),
                registry,
                named->name(),
                flags
            );
        }
    }
    else if (op_type == UnaryOpType::LOGICAL_NOT)
    {
        return std::make_unique<AstPrimitiveType>(
            operation->get_source(),
            operation->get_source_position(),
            registry,
            PrimitiveType::BOOL,
            1
        );
    }

    return type;
}

std::unique_ptr<IAstType> stride::ast::infer_array_member_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    const AstArray* array
)
{
    if (array->get_elements().empty())
    {
        // This is one of those cases where it's impossible to deduce the type
        // Therefore, we have an UNKNOWN type.
        return std::make_unique<AstPrimitiveType>(
            array->get_source(),
            array->get_source_position(),
            registry,
            PrimitiveType::UNKNOWN,
            8, // Always a pointer
            0
        );
    }

    return infer_expression_type(registry, array->get_elements().front().get());
}

std::unique_ptr<IAstType> stride::ast::infer_member_accessor_type(
    [[maybe_unused]] const std::shared_ptr<SymbolRegistry>& registry,
    const AstMemberAccessor* expr
)
{
    // Resolve the Base Identifier (e.g., 'a' in 'a.b.c')
    const auto base_iden = dynamic_cast<AstIdentifier*>(expr->get_base());
    if (!base_iden)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            *expr->get_source(),
            expr->get_source_position()
        );
    }

    // Look up the base variable in the symbol table/registry
    const auto variable_definition = expr->get_registry()->field_lookup(base_iden->get_internal_name());
    if (!variable_definition)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Variable '{}' not found in current sc", base_iden->get_name()
            ),
            *expr->get_source(),
            expr->get_source_position()
        );
    }

    // Start with the type of the base identifier - This will be a struct type
    auto current_type = variable_definition->get_type();

    // Iterate through all member segments (e.g., .b, .c)
    for (const auto& members = expr->get_members();
         const auto member : members)
    {
        // Ensure the current 'node' we are looking inside is actually a struct
        const auto struct_type = dynamic_cast<AstStructType*>(current_type);
        if (!struct_type)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Cannot access member of non-struct type '{}'",
                    current_type->get_internal_name()
                ),
                *expr->get_source(), expr->get_source_position()
            );
        }

        // Root fields, for if the struct is a reference
        auto struct_def = expr->get_registry()->get_struct_def(struct_type->get_internal_name());
        const auto root_ref_struct_fields = expr->get_registry()->get_struct_fields(struct_type->get_internal_name());

        if (!struct_def.has_value() || !root_ref_struct_fields.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Undefined struct '{}'", struct_type->name()),
                *expr->get_source(),
                expr->get_source_position()
            );
        }

        // Resolve the member identifier (e.g., 'b')
        const auto segment_iden = dynamic_cast<AstIdentifier*>(member);
        if (!segment_iden)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                "Member accessor must be an identifier",
                *expr->get_source(),
                expr->get_source_position()
            );
        }

        const auto field_type = StructSymbolDef::get_field_type(
            segment_iden->get_name(),
            root_ref_struct_fields.value()
        );

        if (!field_type.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Variable '{}' of type '{}' has no member named '{}'",
                    base_iden->get_name(),
                    struct_def.value()->get_internal_symbol_name(),
                    segment_iden->get_name()
                ),
                *expr->get_source(),
                expr->get_source_position()
            );
        }

        // Update current_type for the next iteration (or for the final return)
        current_type = field_type.value();
    }

    // Return the final inferred type
    return std::unique_ptr(current_type->clone());
}

std::unique_ptr<IAstType> stride::ast::infer_struct_initializer_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    const AstStructInitializer* initializer
)
{
    return std::make_unique<AstStructType>(
        initializer->get_source(),
        initializer->get_source_position(),
        registry,
        initializer->get_struct_name()
    );
}

std::unique_ptr<IAstType> stride::ast::infer_expression_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    AstExpression* expr
)
{
    if (auto* literal = dynamic_cast<AstLiteral*>(expr))
    {
        return infer_expression_literal_type(registry, literal);
    }

    if (const auto* identifier = dynamic_cast<AstIdentifier*>(expr))
    {
        // TODO: Add generic support.
        // Right now we just do a lookup for `identifier`'s name, though we might want to extend
        // the lookup for generics.
        const auto reference_variable = registry->field_lookup(identifier->get_name());
        if (!reference_variable)
        {
            throw parsing_error(
                ErrorType::SEMANTIC_ERROR,
                std::format("Variable '{}' was not found in this scope", identifier->get_name()),
                *identifier->get_source(),
                identifier->get_source_position()
            );
        }

        return reference_variable->get_type()->clone();
    }

    if (const auto* operation = dynamic_cast<AstBinaryArithmeticOp*>(expr))
    {
        return infer_binary_arithmetic_op_type(registry, operation);
    }

    if (const auto* operation = dynamic_cast<AstUnaryOp*>(expr))
    {
        return infer_unary_op_type(registry, operation);
    }

    if (dynamic_cast<AstLogicalOp*>(expr) || dynamic_cast<AstComparisonOp*>(expr))
    {
        // TODO: Do some validation on lhs and rhs; cannot compare strings with one another (yet)
        return std::make_unique<AstPrimitiveType>(
            expr->get_source(),
            expr->get_source_position(),
            registry,
            PrimitiveType::BOOL,
            1,
            0
        );
    }

    if (const auto* operation = dynamic_cast<AstVariableReassignment*>(expr))
    {
        return infer_expression_type(registry, operation->get_value());
    }

    if (const auto* operation = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        const auto lhs_variable_type = operation->get_variable_type();
        const auto value = infer_expression_type(registry, operation->get_initial_value().get());

        // Both the expression type and the declared type are the same (e.g., let var: i32 = 10)
        // so we can just return the declared type.
        if (lhs_variable_type->equals(*value))
        {
            return lhs_variable_type->clone();
        }

        return get_dominant_field_type(registry, lhs_variable_type, value.get());
    }

    if (const auto* fn_call = dynamic_cast<AstFunctionCall*>(expr))
    {
        return infer_function_call_return_type(registry, fn_call);
    }

    if (const auto* array_expr = dynamic_cast<AstArray*>(expr))
    {
        auto member_type = infer_array_member_type(registry, array_expr);

        return std::make_unique<AstArrayType>(
            array_expr->get_source(),
            array_expr->get_source_position(),
            registry,
            std::move(member_type),
            array_expr->get_elements().size()
        );
    }

    if (const auto* array_accessor = dynamic_cast<AstArrayMemberAccessor*>(expr))
    {
        const auto array_type = infer_expression_type(registry, array_accessor->get_array_identifier());

        if (const auto array = dynamic_cast<AstArrayType*>(array_type.get()); array != nullptr)
        {
            return array->get_element_type()->clone();
        }
    }

    if (const auto* struct_init = dynamic_cast<AstStructInitializer*>(expr))
    {
        return infer_struct_initializer_type(registry, struct_init);
    }

    if (const auto* member_accessor = dynamic_cast<AstMemberAccessor*>(expr))
    {
        return infer_member_accessor_type(registry, member_accessor);
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        "Unable to resolve expression type",
        *expr->get_source(),
        expr->get_source_position()
    );
}
