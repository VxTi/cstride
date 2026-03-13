#include "ast/type_inference.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/flags.h"
#include "ast/parsing_context.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"

#include <memory>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstType> stride::ast::infer_expression_literal_type(const AstLiteral* literal)
{
    return std::make_unique<AstPrimitiveType>(
        literal->get_source_fragment(),
        literal->get_context(),
        literal->get_primitive_type());
}

std::unique_ptr<IAstType> stride::ast::infer_function_call_return_type(AstFunctionCall* fn_call)
{
    /// --- Basic function lookup, find based on parameter signature (ignoring return type)
    const auto& context = fn_call->get_context();

    if (const auto fn_def =
            context->get_function_definition(fn_call->get_scoped_function_name(), fn_call->get_argument_types());
        fn_def.has_value())
    {
        return fn_def.value()->get_type()->get_return_type()->clone_ty();
    }

    /// --- Handling lambda functions that might be assigned to variables
    if (const auto definition = context->lookup_symbol(fn_call->get_function_name()))
    {
        // In case the symbol has a lambda function as value, we'll need to extract it here
        if (const auto field_fn_like_def = dynamic_cast<FieldDefinition*>(definition))
        {
            if (const auto field_fn_type =
                cast_type<AstFunctionType*>(field_fn_like_def->get_type()))
            {
                return field_fn_type->get_return_type()->clone_ty();
            }
            return field_fn_like_def->get_type()->clone_ty();
        }
    }

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        std::format(
            "Function '{}' was not found in this scope",
            fn_call->get_formatted_call()
        ),
        fn_call->get_source_fragment()
    );
}

std::unique_ptr<IAstType> stride::ast::infer_binary_op_type(IBinaryOp* operation)
{
    if (cast_expr<AstLogicalOp*>(operation) || cast_expr<AstComparisonOp*>(operation))
    {
        return std::make_unique<AstPrimitiveType>(
            operation->get_source_fragment(),
            operation->get_context(),
            PrimitiveType::BOOL
        );
    }

    auto lhs = infer_expression_type(operation->get_left());
    auto rhs = infer_expression_type(operation->get_right());

    if (lhs->equals(rhs.get()))
    {
        return std::move(lhs);
    }

    // --- Pointers have priority
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

    if (const auto op_type = operation->get_op_type();
        op_type == UnaryOpType::ADDRESS_OF)
    {
        const auto flags = type->get_flags() | SRFLAG_TYPE_PTR;
        if (const auto* prim = cast_type<AstPrimitiveType*>(type.get()))
        {
            return std::make_unique<AstPrimitiveType>(
                prim->get_source_fragment(),
                context,
                prim->get_primitive_type(),
                flags
            );
        }
        if (const auto* named = cast_type<AstAliasType*>(type.get()))
        {
            return std::make_unique<AstAliasType>(
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
                operation->get_source_fragment()
            );
        }

        return type->clone_ty();
    }
    else if (op_type == UnaryOpType::LOGICAL_NOT)
    {
        return std::make_unique<AstPrimitiveType>(
            operation->get_source_fragment(),
            context,
            PrimitiveType::BOOL
        );
    }

    return type;
}

std::unique_ptr<IAstType> stride::ast::infer_array_member_type(const AstArray* array)
{
    if (array->get_elements().empty())
    {
        // This is one of those cases where it's impossible to deduce the type
        // Therefore, we have an int ptr type.
        return std::make_unique<AstPrimitiveType>(
            array->get_source_fragment(),
            array->get_context(),
            PrimitiveType::INT32,
            SRFLAG_TYPE_PTR
        );
    }

    return infer_expression_type(array->get_elements().front().get());
}

std::unique_ptr<IAstType> stride::ast::infer_chained_expression_type(const AstChainedExpression* chained_expr)
{
    // Infer the type of the base (left side)
    auto base_type = infer_expression_type(chained_expr->get_base());

    // Resolve alias types to get the underlying struct type
    IAstType* struct_type_raw = get_object_type_from_type(base_type.get()).value_or(nullptr);
    if (!struct_type_raw)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Member access base must be a struct type, got '{}'",
                base_type->get_type_name()
            ),
            chained_expr->get_source_fragment()
        );
    }

    const auto* struct_type = dynamic_cast<AstObjectType*>(struct_type_raw);
    if (!struct_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Object type '{}' not found in scope", base_type->get_type_name()),
            chained_expr->get_source_fragment()
        );
    }

    // The followup must be an identifier (the member name)
    const auto* member_id = cast_expr<AstIdentifier*>(chained_expr->get_followup());
    if (!member_id)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Chained expression followup must be an identifier",
            chained_expr->get_source_fragment()
        );
    }

    const auto field_type = struct_type->get_member_field_type(member_id->get_name());
    if (!field_type.has_value())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Field '{}' not found in struct '{}'",
                        member_id->get_name(),
                        base_type->get_type_name()),
            chained_expr->get_source_fragment()
        );
    }

    return field_type.value()->clone_ty();
}

std::unique_ptr<IAstType> stride::ast::infer_indirect_call_type(const AstIndirectCall* call_expr)
{
    auto callee_type = infer_expression_type(call_expr->get_callee());

    // Unwrap alias
    IAstType* raw_type = callee_type.get();
    std::unique_ptr<IAstType> unwrapped;
    if (auto* alias = cast_type<AstAliasType*>(raw_type))
    {
        unwrapped = alias->get_underlying_type()->clone_ty();
        raw_type = unwrapped.get();
    }

    const auto* fn_type = dynamic_cast<AstFunctionType*>(raw_type);
    if (!fn_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Cannot call expression of type '{}' as a function",
                callee_type->get_type_name()
            ),
            call_expr->get_source_fragment()
        );
    }

    return fn_type->get_return_type()->clone_ty();
}

std::unique_ptr<IAstType> stride::ast::infer_object_initializer_type(const AstObjectInitializer* struct_initializer)
{
    GenericTypeList generic_type_arguments;
    generic_type_arguments.reserve(struct_initializer->get_generic_type_arguments().size());
    for (const auto& generic_type_argument : struct_initializer->get_generic_type_arguments())
    {
        generic_type_arguments.emplace_back(generic_type_argument->clone_ty());
    }

    return std::make_unique<AstAliasType>(
        struct_initializer->get_source_fragment(),
        struct_initializer->get_context(),
        struct_initializer->get_struct_name(),
        0,
        std::move(generic_type_arguments)
    );
}

std::unique_ptr<IAstType> stride::ast::infer_function_type(const IAstFunction* expression)
{
    std::vector<std::unique_ptr<IAstType>> param_types;
    param_types.reserve(expression->get_parameters().size());

    for (const auto& param : expression->get_parameters())
    {
        param_types.emplace_back(param->get_type()->clone_ty());
    }

    return std::make_unique<AstFunctionType>(
        expression->get_source_fragment(),
        expression->get_context(),
        std::move(param_types),
        expression->get_return_type()->clone_ty()
    );
}

std::unique_ptr<IAstType> stride::ast::infer_identifier_type(const AstIdentifier* identifier)
{
    const auto identifier_def = identifier->get_definition();

    if (!identifier_def.has_value())
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format(
                "Unable to infer expression type for symbol '{}': symbol not found",
                identifier->get_name()),
            identifier->get_source_fragment());
    }

    if (const auto callable = dynamic_cast<const FunctionDefinition*>(identifier_def.value()))
    {
        std::vector<std::unique_ptr<IAstType>> param_types;
        for (const auto& param :
             callable->get_type()->get_parameter_types())
        {
            param_types.push_back(param->clone_ty());
        }

        return std::make_unique<AstFunctionType>(
            identifier->get_source_fragment(),
            identifier->get_context(),
            std::move(param_types),
            callable->get_type()->get_return_type()->clone_ty()
        );
    }

    if (const auto field = dynamic_cast<const FieldDefinition*>(identifier_def.value()))
    {
        return field->get_type()->clone_ty();
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        std::format(
            "Unable to infer expression type for variable '{}': variable is not a field or "
            "function",
            identifier->get_name()),
        identifier->get_source_fragment());
}

std::unique_ptr<IAstType> stride::ast::infer_variable_declaration_type(
    const AstVariableDeclaration* declaration,
    const int recursion_guard)
{
    const auto annotated_type = declaration->get_annotated_type();
    const auto value_type = infer_expression_type(
        declaration->get_initial_value(),
        recursion_guard
    );
    if (!annotated_type.has_value())
    {
        value_type->set_flags(declaration->get_flags());
        return value_type->clone_ty();
    }

    if (annotated_type.value()->equals(value_type.get()))
    {
        return annotated_type.value()->clone_ty();
    }

    if (value_type->is_assignable_to(annotated_type.value()))
    {
        return get_dominant_field_type(annotated_type.value(), value_type.get());
    }

    const auto references = {
        ErrorSourceReference(
            annotated_type.value()->to_string(),
            annotated_type.value()->get_source_fragment()
        ),
        ErrorSourceReference(
            value_type->to_string(),
            declaration->get_initial_value()->get_source_fragment()
        )
    };

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        std::format(
            "Type mismatch in variable declaration: cannot assign value of type '{}' to type '{}'",
            value_type->to_string(),
            annotated_type.value()->to_string()
        ),
        references
    );
}

std::unique_ptr<IAstType> stride::ast::infer_array_accessor_type(
    const AstArrayMemberAccessor* accessor,
    const int recursion_guard)
{
    // Infer the base expression's type. We must ensure it's an array type, and then we can return the element type.
    const auto array_type = infer_expression_type(accessor->get_array_base(), recursion_guard);

    // If the immediate type is an array, we can simply return the member type
    if (const auto array = cast_type<AstArrayType*>(array_type.get()))
    {
        return array->get_element_type()->clone_ty();
    }

    // It's possible that we're referring to a named type, in which case we'll have to extract the base type
    if (const auto alias_type = cast_type<AstAliasType*>(array_type.get()))
    {
        // Instantiate type if it contains generics
        if (const auto array_base_ty = cast_type<AstArrayType*>(alias_type->get_underlying_type()))
        {
            return array_base_ty->get_element_type()->clone_ty();
        }

        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Named type '{}' references a type that is not an array, cannot be used as array type",
                alias_type->get_name()
            ),
            accessor->get_array_base()->get_source_fragment()
        );
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        std::format("Expected array type for member accessor, got: '{}'",
                    accessor->get_array_base()->get_type()->get_type_name()
        ),
        accessor->get_source_fragment()
    );
}

std::unique_ptr<IAstType> stride::ast::infer_expression_type(IAstExpression* expr, int recursion_guard)
{
    if (!expr)
    {
        throw parsing_error("Expression cannot be null");
    }

    if (++recursion_guard > MAX_RECURSION_DEPTH)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Maximum recursion depth reached while inferring type",
            expr->get_source_fragment()
        );
    }

    if (const auto* type_cast = cast_expr<AstTypeCastOp*>(expr))
    {
        return type_cast->get_target_type()->clone_ty();
    }

    if (const auto* literal = cast_expr<AstLiteral*>(expr))
    {
        return infer_expression_literal_type(literal);
    }

    if (const auto* identifier = cast_expr<AstIdentifier*>(expr))
    {
        return infer_identifier_type(identifier);
    }

    if (auto* operation = cast_expr<IBinaryOp*>(expr))
    {
        return infer_binary_op_type(operation);
    }

    if (const auto* operation = cast_expr<AstUnaryOp*>(expr))
    {
        return infer_unary_op_type(operation);
    }

    if (const auto* operation = cast_expr<AstVariableReassignment*>(expr))
    {
        return infer_expression_type(operation->get_value(), recursion_guard);
    }

    if (const auto* variable_declaration = cast_expr<AstVariableDeclaration*>(expr))
    {
        return infer_variable_declaration_type(variable_declaration, recursion_guard);
    }

    if (auto* fn_call = cast_expr<AstFunctionCall*>(expr))
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
            array_expr->get_elements().size()
        );
    }

    if (const auto* array_accessor = cast_expr<AstArrayMemberAccessor*>(expr))
    {
        return infer_array_accessor_type(array_accessor, recursion_guard);
    }

    if (const auto* struct_init = cast_expr<AstObjectInitializer*>(expr))
    {
        return infer_object_initializer_type(struct_init);
    }

    if (const auto* chained = cast_expr<AstChainedExpression*>(expr))
    {
        return infer_chained_expression_type(chained);
    }

    if (const auto* indirect_call = cast_expr<AstIndirectCall*>(expr))
    {
        return infer_indirect_call_type(indirect_call);
    }

    if (const auto* function_definition = cast_expr<IAstFunction*>(expr))
    {
        return infer_function_type(function_definition);
    }

    if (const auto* tuple_init = cast_expr<AstTupleInitializer*>(expr))
    {
        std::vector<std::unique_ptr<IAstType>> param_types;
        param_types.reserve(tuple_init->get_members().size());

        for (const auto& member : tuple_init->get_members())
        {
            param_types.emplace_back(infer_expression_type(member.get(), recursion_guard));
        }

        return std::make_unique<AstTupleType>(
            tuple_init->get_source_fragment(),
            tuple_init->get_context(),
            std::move(param_types)
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
            SRFLAG_TYPE_PTR
        );
    }

    throw parsing_error(
        ErrorType::SEMANTIC_ERROR,
        "Unable to resolve expression type",
        expr->get_source_fragment()
    );
}
