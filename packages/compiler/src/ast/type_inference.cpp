#include "ast/casting.h"
#include "ast/flags.h"
#include "ast/parsing_context.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

#include <memory>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstType> stride::ast::infer_expression_literal_type(AstLiteral* literal)
{
    const auto& context = literal->get_context();
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
    const AstFunctionCall* fn_call)
{
    const auto& context = fn_call->get_context();
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
    const AstBinaryArithmeticOp* operation)
{

    auto lhs = infer_expression_type(operation->get_left());
    auto rhs = infer_expression_type(operation->get_right());

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

    return get_dominant_field_type(lhs.get(), rhs.get());
}

std::unique_ptr<IAstType> stride::ast::infer_unary_op_type(const AstUnaryOp* operation)
{
    const auto& context = operation->get_context();
    auto type = infer_expression_type(&operation->get_operand());

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
                named->get_name(),
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
                named->get_name(),
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

std::unique_ptr<IAstType> stride::ast::infer_array_member_type(const AstArray* array)
{
    if (array->get_elements().empty())
    {
        // This is one of those cases where it's impossible to deduce the type
        // Therefore, we have an UNKNOWN type.
        return std::make_unique<AstPrimitiveType>(
            array->get_source_fragment(),
            array->get_context(),
            PrimitiveType::UNKNOWN,
            8,
            // Always a pointer
            0);
    }

    return infer_expression_type(array->get_elements().front().get());
}

std::unique_ptr<IAstType> stride::ast::infer_member_accessor_type(
    const AstMemberAccessor* member_accessor_expr)
{
    // Base must be an identifier, e.g., <identifier>.<member1>...
    const auto base_iden = cast_expr<AstIdentifier*>(member_accessor_expr->get_base());
    if (!base_iden)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            member_accessor_expr->get_source_fragment());
    }

    // ---- Look up the base variable in the symbol table/context
    const auto variable_definition = member_accessor_expr->get_context()->lookup_variable(
        base_iden->get_name(),
        true);

    if (!variable_definition)
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Variable '{}' not found in scope",
                        base_iden->get_name()),
            member_accessor_expr->get_source_fragment());
    }

    // ---- Ensure parent (base) is a struct type
    IAstType* parent_type = nullptr;

    if (const auto struct_ty = cast_type<AstStructType*>(variable_definition->get_type()))
    {
        parent_type = struct_ty;
    }
    else if (const auto named_ty = cast_type<AstNamedType*>(variable_definition->get_type()))
    {
        // Lookup struct type definition for named type reference
        const auto struct_def =
            member_accessor_expr->get_context()->get_struct_type(named_ty->get_name());
        parent_type = cast_type<AstStructType*>(struct_def.value_or(nullptr));
    }

    if (!parent_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Struct type '{}' not found in this scope",
                variable_definition->get_type()->get_type_name()),
            member_accessor_expr->get_source_fragment());
    }

    // Iterate through all member segments (e.g., .b, .c)
    for (const auto& members = member_accessor_expr->get_members();
         const auto member : members)
    {
        auto struct_type = get_struct_type(parent_type);

        // It's possible that the previous iteration yielded a non-struct type, and thus being nullptr.
        if (!struct_type.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Struct type '{}' not found in this scope",
                    parent_type->get_type_name()),
                member_accessor_expr->get_source_fragment());
        }

        // Resolve the member identifier (e.g., 'b')
        // For now, members are already identifiers, though this might change in the future.

        const auto member_identifier = cast_expr<AstIdentifier*>(member);
        if (!member_identifier)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                "Member accessor must be an identifier",
                member_accessor_expr->get_source_fragment());
        }

        const auto field_type = struct_type.value()->get_member_field_type(
            member_identifier->get_name());

        if (!field_type.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Field '{}' not found in struct",
                    member_identifier->get_name()),
                member_accessor_expr->get_source_fragment());
        }

        // Update current_type for the next iteration (or for the final return)
        parent_type = field_type.value();
    }

    // Return the final inferred type
    return std::unique_ptr(parent_type->clone());
}

std::unique_ptr<IAstType> stride::ast::infer_struct_initializer_type(
    const AstStructInitializer* initializer)
{
    return std::make_unique<AstNamedType>(
        initializer->get_source_fragment(),
        initializer->get_context(),
        initializer->get_struct_name());
}

std::unique_ptr<IAstType> stride::ast::infer_expression_type(AstExpression* expr)
{
    if (!expr)
    {
        throw parsing_error("Expression cannot be null");
    }


    if (auto* literal = cast_expr<AstLiteral*>(expr))
    {
        return infer_expression_literal_type(literal);
    }

    if (const auto* identifier = cast_expr<AstIdentifier*>(expr))
    {
        // TODO: Add generic support.
        // Right now we just do a lookup for `identifier`'s name, though we might want to extend
        // the lookup for generics.
        const auto reference_sym = identifier->get_context()->lookup_symbol(
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
            for (const auto& param :
                 callable->get_type()->get_parameter_types())
            {
                param_types.push_back(param->clone());
            }

            return std::make_unique<AstFunctionType>(
                identifier->get_source_fragment(),
                identifier->get_context(),
                std::move(param_types),
                callable->get_type()->get_return_type()->clone()
            );
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
        return infer_binary_arithmetic_op_type(operation);
    }

    if (const auto* operation = cast_expr<AstUnaryOp*>(expr))
    {
        return infer_unary_op_type(operation);
    }

    if (cast_expr<AstLogicalOp*>(expr) || cast_expr<AstComparisonOp*>(expr))
    {
        // TODO: Do some validation on lhs and rhs; cannot compare strings with one another (yet)
        return std::make_unique<AstPrimitiveType>(
            expr->get_source_fragment(),
            expr->get_context(),
            PrimitiveType::BOOL,
            1,
            0);
    }

    if (const auto* operation = cast_expr<AstVariableReassignment*>(expr))
    {
        return infer_expression_type(operation->get_value());
    }

    if (const auto* operation = cast_expr<AstVariableDeclaration*>(expr))
    {
        const auto lhs_variable_type = operation->get_variable_type();
        const auto value = infer_expression_type(
            operation->get_initial_value().get());

        // Both the expression type and the declared type are the same (e.g., let var: int32 = 10)
        // so we can just return the declared type.
        if (lhs_variable_type->equals(*value))
        {
            return lhs_variable_type->clone();
        }

        return get_dominant_field_type(lhs_variable_type, value.get());
    }

    if (const auto* fn_call = cast_expr<AstFunctionCall*>(expr))
    {
        return infer_function_call_return_type(fn_call);
    }

    if (const auto* array_expr = cast_expr<AstArray*>(expr))
    {
        auto member_type = infer_array_member_type(array_expr);

        return std::make_unique<AstArrayType>(
            array_expr->get_source_fragment(),
            array_expr->get_context(),
            std::move(member_type),
            array_expr->get_elements().size());
    }

    if (const auto* array_accessor = cast_expr<AstArrayMemberAccessor*>(expr))
    {
        const auto array_type =
            infer_expression_type(array_accessor->get_array_identifier());

        if (const auto array = cast_type<AstArrayType*>(array_type.get()); array
            != nullptr)
        {
            return array->get_element_type()->clone();
        }
    }

    if (const auto* struct_init = cast_expr<AstStructInitializer*>(expr))
    {
        return infer_struct_initializer_type(struct_init);
    }

    if (const auto* member_accessor = cast_expr<AstMemberAccessor*>(expr))
    {
        return infer_member_accessor_type(member_accessor);
    }

    if (const auto* function_definition = cast_expr<AstLambdaFunctionExpression*>(expr))
    {
        std::vector<std::unique_ptr<IAstType>> param_types;

        for (const auto& param : function_definition->get_parameters())
        {
            param_types.emplace_back(param->get_type()->clone());
        }

        return std::make_unique<AstFunctionType>(
            function_definition->get_source_fragment(),
            function_definition->get_context(),
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
            expr->get_context(),
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
