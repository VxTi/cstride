#include "ast/casting.h"
#include "ast/flags.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

#include <memory>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstType> stride::ast::infer_expression_literal_type(
    const std::shared_ptr<ParsingContext>& context,
    AstLiteral* literal)
{
    if (const auto* str = cast_expr<AstStringLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(
            str->get_source_fragment(),
            context,
            PrimitiveType::STRING,
            1);
    }

    if (const auto* fp_lit = cast_expr<AstFpLiteral*>(literal))
    {
        auto type = fp_lit->bit_count() > 32
            ? PrimitiveType::FLOAT64
            : PrimitiveType::FLOAT32;
        return std::make_unique<AstPrimitiveType>(
            fp_lit->get_source_fragment(),
            context,
            type,
            fp_lit->bit_count());
    }

    if (const auto* int_lit = cast_expr<AstIntLiteral*>(literal))
    {
        auto type = int_lit->is_signed()
            ? (int_lit->bit_count() > 32
                ? PrimitiveType::INT64
                : PrimitiveType::INT32)
            : (int_lit->bit_count() > 32
                ? PrimitiveType::UINT64
                : PrimitiveType::UINT32);

        return std::make_unique<AstPrimitiveType>(

            int_lit->get_source_fragment(),
            context,
            type,
            int_lit->bit_count(),
            int_lit->get_flags());
    }

    if (const auto* char_lit = cast_expr<AstCharLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(

            char_lit->get_source_fragment(),
            context,
            PrimitiveType::CHAR,
            char_lit->bit_count());
    }

    if (const auto* bool_lit = cast_expr<AstBooleanLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(

            bool_lit->get_source_fragment(),
            context,
            PrimitiveType::BOOL,
            bool_lit->bit_count());
    }

    if (const auto* nil_lit = cast_expr<AstNilLiteral*>(literal))
    {
        return std::make_unique<AstPrimitiveType>(

            nil_lit->get_source_fragment(),
            context,
            PrimitiveType::NIL,
            8);
    }

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        "Unable to resolve expression literal type",
        literal->get_source_fragment());
}

std::unique_ptr<IAstType> stride::ast::infer_function_call_return_type(
    const std::shared_ptr<ParsingContext>& context,
    const AstFunctionCall* fn_call)
{
    // First steps, if the function call references a "normal" function, e.g., "fn <some name>",
    // then we can simply use
    if (const auto fn_def = context->get_function_def(
        fn_call->get_internal_name()))
    {
        return fn_def->get_type()->get_return_type()->clone();
    }

    // It could be an extern function, in which case the function name is just as-is
    if (const auto fn_def = context->get_function_def(
        fn_call->get_function_name()))
    {
        return fn_def->get_type()->get_return_type()->clone();
    }
    std::vector candidates = { fn_call->get_internal_name(),
                               fn_call->get_function_name() };

    // It might also be a field that's assigned a function
    if (const auto definition = context->lookup_symbol(
        fn_call->get_function_name()))
    {
        // Simple extraction. it's already referencing a 'real' function.
        if (const auto callable = dynamic_cast<CallableDef*>(definition))
        {
            return callable->get_type()->get_return_type()->clone();
        }
        // In case the symbol has a lambda function as value, we'll need to extract it here
        if (const auto field_fn_like_def = dynamic_cast<FieldDef*>(definition))
        {
            if (const auto field_fn_type =
                cast_type<AstFunctionType*>(field_fn_like_def->get_type()))
            {
                return field_fn_type->get_return_type()->clone();
            }
            return field_fn_like_def->get_type()->clone();
        }
    }

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        std::format(
            "Unable to resolve return type for function '{}'",
            fn_call->get_function_name()),
        fn_call->get_source_fragment());
}

std::unique_ptr<IAstType> stride::ast::infer_binary_arithmetic_op_type(
    const std::shared_ptr<ParsingContext>& context,
    const AstBinaryArithmeticOp* operation)
{
    auto lhs = infer_expression_type(context, operation->get_left());
    auto rhs = infer_expression_type(context, operation->get_right());

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

    return get_dominant_field_type(context, lhs.get(), rhs.get());
}

std::unique_ptr<IAstType> stride::ast::infer_unary_op_type(
    const std::shared_ptr<ParsingContext>& context,
    const AstUnaryOp* operation)
{
    auto type = infer_expression_type(context, &operation->get_operand());

    if (const auto op_type = operation->get_op_type(); op_type ==
        UnaryOpType::ADDRESS_OF)
    {
        const auto flags = type->get_flags() | SRFLAG_TYPE_PTR;
        if (const auto* prim = cast_type<AstPrimitiveType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveType>(
                prim->get_source_fragment(),
                context,
                prim->get_type(),
                prim->bit_count(),
                flags);
        }
        if (const auto* named = cast_type<AstNamedType*>(type.get()))
        {
            return std::make_unique<AstNamedType>(
                named->get_source_fragment(),
                context,
                named->name(),
                flags);
        }
    }
    else if (op_type == UnaryOpType::DEREFERENCE)
    {
        if (!type->is_pointer())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                "Cannot dereference non-pointer type",
                operation->get_source_fragment());
        }

        const auto flags = type->get_flags() & ~SRFLAG_TYPE_PTR;

        if (const auto* prim = cast_type<AstPrimitiveType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveType>(
                prim->get_source_fragment(),
                context,
                prim->get_type(),
                prim->bit_count(),
                flags);
        }
        if (const auto* named = cast_type<AstNamedType*>(type.get()))
        {
            return std::make_unique<AstNamedType>(
                named->get_source_fragment(),
                context,
                named->name(),
                flags);
        }
    }
    else if (op_type == UnaryOpType::LOGICAL_NOT)
    {
        return std::make_unique<AstPrimitiveType>(
            operation->get_source_fragment(),
            context,
            PrimitiveType::BOOL,
            1);
    }

    return type;
}

std::unique_ptr<IAstType> stride::ast::infer_array_member_type(
    const std::shared_ptr<ParsingContext>& context,
    const AstArray* array)
{
    if (array->get_elements().empty())
    {
        // This is one of those cases where it's impossible to deduce the type
        // Therefore, we have an UNKNOWN type.
        return std::make_unique<AstPrimitiveType>(
            array->get_source_fragment(),
            context,
            PrimitiveType::UNKNOWN,
            8,
            // Always a pointer
            0);
    }

    return infer_expression_type(context, array->get_elements().front().get());
}

std::unique_ptr<IAstType> stride::ast::infer_member_accessor_type(
    [[maybe_unused]] const std::shared_ptr<ParsingContext>& context,
    const AstMemberAccessor* expr)
{
    // Resolve the Base Identifier (e.g., 'a' in 'a.b.c')
    const auto base_iden = cast_expr<AstIdentifier*>(expr->get_base());
    if (!base_iden)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            expr->get_source_fragment());
    }

    // Look up the base variable in the symbol table/context
    const auto variable_definition = expr->get_context()->lookup_variable(
        base_iden->get_name());
    if (!variable_definition)
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Variable '{}' not found in scope",
                        base_iden->get_name()),
            expr->get_source_fragment());
    }

    // Start with the type of the base identifier - This will be a struct type
    auto current_type = variable_definition->get_type();

    // Iterate through all member segments (e.g., .b, .c)
    for (const auto& members = expr->get_members(); const auto member : members)
    {
        // Ensure the current 'node' we are looking inside is actually a struct
        const auto struct_type = cast_type<AstNamedType*>(current_type);
        if (!struct_type)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Cannot access member of non-struct type '{}'",
                    current_type->get_internal_name()),
                expr->get_source_fragment());
        }

        // Root fields, for if the struct is a reference
        auto struct_def = expr->get_context()->get_struct_def(
            struct_type->get_internal_name());
        const auto root_ref_struct_fields =
            expr->get_context()->get_struct_fields(
                struct_type->get_internal_name());

        if (!struct_def.has_value() || !root_ref_struct_fields.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Undefined struct '{}'", struct_type->name()),
                expr->get_source_fragment());
        }

        // Resolve the member identifier (e.g., 'b')
        const auto segment_iden = cast_expr<AstIdentifier*>(member);
        if (!segment_iden)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                "Member accessor must be an identifier",
                expr->get_source_fragment());
        }

        const auto field_type = StructDef::get_struct_member_field_type(
            segment_iden->get_name(),
            root_ref_struct_fields.value());

        if (!field_type.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Variable '{}' of type '{}' has no member named '{}'",
                    base_iden->get_name(),
                    struct_def.value()->get_internal_symbol_name(),
                    segment_iden->get_name()),
                expr->get_source_fragment());
        }

        // Update current_type for the next iteration (or for the final return)
        current_type = field_type.value();
    }

    // Return the final inferred type
    return std::unique_ptr(current_type->clone());
}

std::unique_ptr<IAstType> stride::ast::infer_struct_initializer_type(
    const std::shared_ptr<ParsingContext>& context,
    const AstStructInitializer* initializer)
{
    return std::make_unique<AstNamedType>(

        initializer->get_source_fragment(),
        context,
        initializer->get_struct_name());
}

std::unique_ptr<IAstType> stride::ast::infer_expression_type(
    const std::shared_ptr<ParsingContext>& context,
    AstExpression* expr
)
{
    if (!expr)
    {
        throw parsing_error("Expression cannot be null");
    }
    if (auto* literal = cast_expr<AstLiteral*>(expr))
    {
        return infer_expression_literal_type(context, literal);
    }

    if (const auto* identifier = cast_expr<AstIdentifier*>(expr))
    {
        // TODO: Add generic support.
        // Right now we just do a lookup for `identifier`'s name, though we might want to extend
        // the lookup for generics.
        const auto reference_sym = context->lookup_symbol(
            identifier->get_name());
        if (!reference_sym)
        {
            throw parsing_error(
                ErrorType::REFERENCE_ERROR,
                std::format(
                    "Unable to infer expression type for '{}': variable or function not found",
                    identifier->get_name()),
                identifier->get_source_fragment());
        }

        if (const auto callable = dynamic_cast<CallableDef*>(reference_sym))
        {
            std::vector<std::unique_ptr<IAstType>> param_types;
            for (const auto& param : callable->get_type()->
                                               get_parameter_types())
            {
                param_types.push_back(param->clone());
            }

            return std::make_unique<AstFunctionType>(

                identifier->get_source_fragment(),
                context,
                std::move(param_types),
                callable->get_type()->get_return_type()->clone());
        }

        if (const auto field = dynamic_cast<FieldDef*>(reference_sym))
        {
            return field->get_type()->clone();
        }

        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format(
                "Unable to infer expression type for variable '{}': variable is not a field or "
                "function",
                identifier->get_name()),
            identifier->get_source_fragment());
    }

    if (const auto* operation = cast_expr<AstBinaryArithmeticOp*>(expr))
    {
        return infer_binary_arithmetic_op_type(context, operation);
    }

    if (const auto* operation = cast_expr<AstUnaryOp*>(expr))
    {
        return infer_unary_op_type(context, operation);
    }

    if (cast_expr<AstLogicalOp*>(expr) || cast_expr<AstComparisonOp*>(expr))
    {
        // TODO: Do some validation on lhs and rhs; cannot compare strings with one another (yet)
        return std::make_unique<AstPrimitiveType>(

            expr->get_source_fragment(),
            context,
            PrimitiveType::BOOL,
            1,
            0);
    }

    if (const auto* operation = cast_expr<AstVariableReassignment*>(expr))
    {
        return infer_expression_type(context, operation->get_value());
    }

    if (const auto* operation = cast_expr<AstVariableDeclaration*>(expr))
    {
        const auto lhs_variable_type = operation->get_variable_type();
        const auto value = infer_expression_type(
            context,
            operation->get_initial_value().get());

        // Both the expression type and the declared type are the same (e.g., let var: int32 = 10)
        // so we can just return the declared type.
        if (lhs_variable_type->equals(*value))
        {
            return lhs_variable_type->clone();
        }

        return get_dominant_field_type(context, lhs_variable_type, value.get());
    }

    if (const auto* fn_call = cast_expr<AstFunctionCall*>(expr))
    {
        return infer_function_call_return_type(context, fn_call);
    }

    if (const auto* array_expr = cast_expr<AstArray*>(expr))
    {
        auto member_type = infer_array_member_type(context, array_expr);

        return std::make_unique<AstArrayType>(
            array_expr->get_source_fragment(),
            context,
            std::move(member_type),
            array_expr->get_elements().size());
    }

    if (const auto* array_accessor = cast_expr<AstArrayMemberAccessor*>(expr))
    {
        const auto array_type =
            infer_expression_type(context,
                                  array_accessor->get_array_identifier());

        if (const auto array = cast_type<AstArrayType*>(array_type.get()); array
            != nullptr)
        {
            return array->get_element_type()->clone();
        }
    }

    if (const auto* struct_init = cast_expr<AstStructInitializer*>(expr))
    {
        return infer_struct_initializer_type(context, struct_init);
    }

    if (const auto* member_accessor = cast_expr<AstMemberAccessor*>(expr))
    {
        return infer_member_accessor_type(context, member_accessor);
    }

    if (const auto* function_definition = cast_expr<AstLambdaFunctionExpression
        *>(expr))
    {
        std::vector<std::unique_ptr<IAstType>> param_types;

        for (const auto& param : function_definition->get_parameters())
        {
            param_types.emplace_back(param->get_type()->clone());
        }

        return std::make_unique<AstFunctionType>(
            static_cast<const AstExpression*>(function_definition)->get_source_fragment(),
            context,
            std::move(param_types),
            function_definition->get_return_type()->clone()
        );
    }

    if (cast_expr<AstVariadicArgReference*>(expr))
    {
        // Variadic argument reference (...) represents a va_list in LLVM
        // We return a pointer type (i8*) to represent the va_list pointer
        return std::make_unique<AstPrimitiveType>(
            expr->get_source_fragment(),
            context,
            PrimitiveType::INT8,
            8,
            SRFLAG_TYPE_PTR
        );
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        "Unable to resolve expression type",
        expr->get_source_fragment()
    );
}
