#include "ast/casting.h"
#include "ast/definitions/function_definition.h"
#include "ast/generics.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/while_loop.h"

#include <ranges>
#include <llvm/IR/Module.h>

using namespace stride::ast;

/**
 * Here we define the function in the symbol table, so it can be looked up in the codegen phase.
 */
void IAstFunction::resolve_forward_references(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    std::vector<Symbol> captures;
    const auto outer_context = this->get_context()->get_parent_context() != nullptr
        ? this->get_context()->get_parent_context()
        : this->get_context();

    collect_free_variables(this->get_body(), this->get_context(), outer_context, captures);

    // Register captured variables in the lambda's context so they can be referenced
    for (const auto& capture : captures)
    {
        this->add_captured_variable(capture);

        // Also define the capture in the lambda's context so identifier lookup works
        if (const auto outer_var = this->get_context()->lookup_variable(capture.name, true))
        {
            this->get_context()->define_variable(
                capture,
                outer_var->get_type()->clone_ty(),
                VisibilityModifier::PRIVATE
            );
        }
    }

    // Avoid re-registering if already declared.
    // Named functions are looked up by their scoped name; anonymous functions are
    // tracked by the cached _llvm_function pointer (they have no stable string name).
    if (this->is_anonymous() && this->get_function_definition()->get_llvm_function() != nullptr)
        return;

    // Add captured variables as first parameters
    std::vector<llvm::Type*> captured_types;
    for (const auto& capture : this->_captured_variables)
    {
        if (const auto capture_def = this->get_context()->lookup_variable(capture.name, true))
        {
            if (llvm::Type* capture_type = capture_def->get_type()->get_llvm_type(module))
            {
                captured_types.push_back(capture_type);
            }
        }
    }

    const auto& definition = this->get_function_definition();

    const auto linkage = this->_visibility == VisibilityModifier::PRIVATE
        ? llvm::Function::PrivateLinkage
        : llvm::Function::ExternalLinkage;

    if (!this->is_generic())
    {
        llvm::FunctionType* generic_function_type = this->get_llvm_function_type(
            module,
            captured_types,
            {}
        );

        definition->set_llvm_function(llvm::Function::Create(
            generic_function_type,
            linkage,
            this->get_registered_function_name(),
            module
        ));
        this->_body->resolve_forward_references(module, builder);

        return;
    }

    if (const auto& overloads = definition->get_generic_overloads();
        overloads.empty())
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Generic function '{}' has no instantiations", this->get_plain_function_name()),
            this->get_source_fragment()
        );
    }

    for (const auto& [instantiated_generic_types, llvm_function, node] : definition->get_generic_overloads())
    {
        auto instantiated_return_ty = resolve_generics(
            this->_annotated_return_type.get(),
            this->_generic_parameters,
            instantiated_generic_types
        );

        std::vector<std::unique_ptr<AstFunctionParameter>> instantiated_function_params;
        instantiated_function_params.reserve(this->_parameters.size());

        for (const auto& param : this->_parameters)
        {
            instantiated_function_params.push_back(
                std::make_unique<AstFunctionParameter>(
                    param->get_source_fragment(),
                    param->get_context(),
                    param->get_name(),
                    resolve_generics(param->get_type(), this->_generic_parameters, instantiated_generic_types)
                )
            );
        }

        auto resolved_body = this->_body->clone_as<AstBlock>();
        resolve_generics_in_body(
            resolved_body.get(),
            this->_generic_parameters,
            instantiated_generic_types
        );

        node = std::make_unique<AstFunctionDeclaration>(
            this->get_context(),
            this->get_symbol(),
            std::move(instantiated_function_params),
            std::move(resolved_body),
            std::move(instantiated_return_ty),
            this->get_visibility(),
            this->get_flags(),
            EMPTY_GENERIC_PARAMETER_LIST
        );

        const auto overloaded_fn_name = get_overloaded_function_name(
            this->get_registered_function_name(),
            instantiated_generic_types);
        llvm::FunctionType* generic_function_type = this->get_llvm_function_type(
            module,
            captured_types,
            instantiated_generic_types
        );

        llvm_function = llvm::Function::Create(
            generic_function_type,
            linkage,
            overloaded_fn_name,
            module
        );

        for (const auto& instantiated_param : instantiated_function_params)
        {
            const auto param_symbol = Symbol(instantiated_param->get_source_fragment(), instantiated_param->get_name());
            instantiated_param->get_context()->define_variable(
                param_symbol,
                instantiated_param->get_type()->clone_ty(),
                VisibilityModifier::PRIVATE,
                true
            );
        }

        if (this->is_anonymous())
            llvm_function->addFnAttr("stride.anonymous");

        node->get_body()->resolve_forward_references(module, builder);
    }
}

void IAstFunction::collect_free_variables(
    IAstNode* node,
    const std::shared_ptr<ParsingContext>& lambda_context,
    const std::shared_ptr<ParsingContext>& outer_context,
    std::vector<Symbol>& captures
)
{
    if (!node)
    {
        return;
    }

    auto capture_variable = [&](const std::string& name)
    {
        if (lambda_context->get_variable_def(name, true))
        {
            // No need to collect any variables; they're readily available
            return;
        }

        // Not local to the lambda - check if it's in an outer scope (and is a variable, not a function)
        if (const auto outer_symbol = outer_context->lookup_variable(name, true))
        {
            // Check if we haven't already captured this variable
            bool already_captured = false;
            for (const auto& cap : captures)
            {
                if (cap.internal_name == outer_symbol->get_internal_symbol_name())
                {
                    already_captured = true;
                    break;
                }
            }

            if (!already_captured)
            {
                captures.push_back(outer_symbol->get_symbol());
            }
        }
    };

    // Check specific node types first, then fall back to generic container handling

    // If it's an identifier, check if it references a variable from outer scope
    if (const auto* identifier = cast_expr<const AstIdentifier*>(node))
    {
        capture_variable(identifier->get_name());
        return;
    }

    // Handle nested callables (lambdas) - recursively collect their free variables
    if (auto* callable = cast_expr<IAstFunction*>(node))
    {
        // For nested lambdas, we need to:
        // 1. First collect what the nested lambda needs from its body (if not already done)
        // 2. Then capture those variables that are available in our outer context

        // Check if this nested lambda already has captures collected
        // If it does, we just need to propagate them upward

        if (const auto& existing_captures = callable->get_captured_variables();
            existing_captures.empty() && callable->get_body())
        {
            // Captures haven't been collected yet, so do it now
            std::vector<Symbol> nested_captures;

            // Recursively collect free variables in the nested lambda's body
            // The nested lambda's context is its own context, and its outer context is our lambda_context
            collect_free_variables(
                callable->get_body(),
                callable->get_context(),
                lambda_context,
                nested_captures);

            // Now register the nested lambda's captures
            for (const auto& nested_capture : nested_captures)
            {
                callable->add_captured_variable(nested_capture);

                // Define the capture in the nested lambda's context so identifier lookup works
                if (const auto var_def = lambda_context->lookup_variable(nested_capture.name, true))
                {
                    callable->get_context()->define_variable(
                        nested_capture,
                        var_def->get_type()->clone_ty(),
                        VisibilityModifier::PRIVATE
                    );
                }
            }
        }

        // Now capture those variables into OUR scope if they come from outside
        for (const auto& cap : callable->get_captured_variables())
        {
            capture_variable(cap.name);
        }
        return;
    }

    // Handle return statements
    if (const auto* return_stmt = cast_ast<AstReturnStatement*>(node))
    {
        if (return_stmt->get_return_expression().has_value())
        {
            collect_free_variables(
                return_stmt->get_return_expression().value().get(),
                lambda_context,
                outer_context,
                captures);
        }
        return;
    }

    // Handle function calls
    if (const auto* fn_call = cast_expr<AstFunctionCall*>(node))
    {
        // Capture the "function name" if it's actually a variable (anonymous call)
        capture_variable(fn_call->get_function_name());

        for (const auto& arg : fn_call->get_arguments())
        {
            collect_free_variables(arg.get(), lambda_context, outer_context, captures);
        }
        return;
    }

    // Handle variable declarations (initializer)
    if (const auto* var_decl = cast_expr<AstVariableDeclaration*>(node))
    {
        if (var_decl->get_initial_value())
        {
            collect_free_variables(
                var_decl->get_initial_value(),
                lambda_context,
                outer_context,
                captures);
        }
        return;
    }

    // Handle variable reassignment
    if (const auto* assignment = cast_expr<AstVariableReassignment*>(node))
    {
        collect_free_variables(assignment->get_value(), lambda_context, outer_context, captures);
        return;
    }

    // Handle binary ops
    if (const auto* bin_op = cast_expr<IBinaryOp*>(node))
    {
        collect_free_variables(bin_op->get_left(), lambda_context, outer_context, captures);
        collect_free_variables(bin_op->get_right(), lambda_context, outer_context, captures);
        return;
    }

    // Handle unary ops
    if (const auto* unary_op = cast_expr<AstUnaryOp*>(node))
    {
        collect_free_variables(&unary_op->get_operand(), lambda_context, outer_context, captures);
        return;
    }

    // Handle if statements
    if (auto* if_stmt = cast_ast<AstConditionalStatement*>(node))
    {
        collect_free_variables(if_stmt->get_condition(), lambda_context, outer_context, captures);
        collect_free_variables(if_stmt->get_body(), lambda_context, outer_context, captures);
        if (if_stmt->get_else_body())
        {
            collect_free_variables(
                if_stmt->get_else_body(),
                lambda_context,
                outer_context,
                captures);
        }
        return;
    }

    // Handle while loops
    if (auto* while_loop = cast_ast<AstWhileLoop*>(node))
    {
        collect_free_variables(
            while_loop->get_condition(),
            lambda_context,
            outer_context,
            captures);
        collect_free_variables(while_loop->get_body(), lambda_context, outer_context, captures);
        return;
    }

    // Handle for loops
    if (auto* for_loop = cast_ast<AstForLoop*>(node))
    {
        if (for_loop->get_initializer())
        {
            collect_free_variables(
                for_loop->get_initializer(),
                lambda_context,
                outer_context,
                captures);
        }
        if (for_loop->get_condition())
        {
            collect_free_variables(
                for_loop->get_condition(),
                lambda_context,
                outer_context,
                captures);
        }
        if (for_loop->get_incrementor())
        {
            collect_free_variables(
                for_loop->get_incrementor(),
                lambda_context,
                outer_context,
                captures);
        }

        collect_free_variables(for_loop->get_body(), lambda_context, outer_context, captures);
        return;
    }

    // Handle array literals
    if (const auto* array = cast_expr<AstArray*>(node))
    {
        for (const auto& elem : array->get_elements())
        {
            collect_free_variables(elem.get(), lambda_context, outer_context, captures);
        }
        return;
    }

    // Handle struct initializers
    if (const auto* struct_init = cast_expr<AstObjectInitializer*>(node))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
        {
            collect_free_variables(val.get(), lambda_context, outer_context, captures);
        }
        return;
    }

    // Handle chained member access
    if (const auto* chained = cast_expr<AstChainedExpression*>(node))
    {
        collect_free_variables(chained->get_base(), lambda_context, outer_context, captures);
        return;
    }

    // Handle blocks (lambda bodies, function bodies, etc.)
    if (const auto* block = cast_ast<AstBlock*>(node))
    {
        for (const auto& child : block->get_children())
        {
            collect_free_variables(child.get(), lambda_context, outer_context, captures);
        }
        return;
    }

    // Generic container handling (if statements, loops, etc.) - this should be last
    if (auto* container = dynamic_cast<IAstContainer*>(node))
    {
        if (const auto* body = container->get_body())
        {
            for (const auto& child : body->get_children())
            {
                collect_free_variables(child.get(), lambda_context, outer_context, captures);
            }
        }
    }
}

llvm::FunctionType* IAstFunction::get_llvm_function_type(
    llvm::Module* module,
    std::vector<llvm::Type*> captured_variables,
    const GenericTypeList& generic_instantiation_types
) const
{
    std::vector<llvm::Type*> base_parameter_types;

    if (generic_instantiation_types.empty())
    {
        for (const auto& param : this->_parameters)
        {
            if (llvm::Type* param_type = param->get_type()->get_llvm_type(module))
            {
                base_parameter_types.push_back(param_type);
            }
        }
    }
    else
    {
        for (const auto& param : this->_parameters)
        {
            const auto& resolved_generic_param_type = resolve_generics(
                param->get_type(),
                this->_generic_parameters,
                generic_instantiation_types
            );
            if (llvm::Type* param_type = resolved_generic_param_type->get_llvm_type(module))
            {
                base_parameter_types.push_back(param_type);
            }
        }
    }

    std::vector<llvm::Type*> parameter_types;
    parameter_types.reserve(base_parameter_types.size() + captured_variables.size());

    // Captured variables are first in the internal LLVM function type
    parameter_types.insert(parameter_types.end(), captured_variables.begin(), captured_variables.end());
    parameter_types.insert(parameter_types.end(), base_parameter_types.begin(), base_parameter_types.end());

    llvm::Type* return_type;
    if (!generic_instantiation_types.empty())
    {
        auto resolved_return_ty = resolve_generics(
            this->_annotated_return_type.get(),
            this->_generic_parameters,
            generic_instantiation_types
        );
        return_type = resolved_return_ty->get_llvm_type(module);
    }
    else
    {
        return_type = this->get_return_type()->get_llvm_type(module);
    }

    if (!return_type)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Could not get LLVM return type for function: " + this->get_plain_function_name(),
            this->get_source_fragment()
        );
    }

    const auto& llvm_function_ty = llvm::FunctionType::get(return_type, parameter_types, this->is_variadic());;

    if (const auto fn = module->getFunction(this->get_registered_function_name());
        fn != nullptr && fn->getFunctionType() != llvm_function_ty)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "Function symbol '{}' already exists with a different signature",
                this->get_registered_function_name()
            ),
            this->get_source_fragment()
        );
    }

    return llvm_function_ty;
}
