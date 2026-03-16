#include "ast/nodes/function_definition.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/closures.h"
#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/while_loop.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <iostream>
#include <ranges>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

using namespace stride::ast;
using namespace stride::ast::definition;

/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDeclaration> stride::ast::parse_fn_declaration(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    VisibilityModifier modifier
)
{
    int function_flags = 0;
    if (set.peek_next_eq(TokenType::KEYWORD_EXTERN))
    {
        set.next();
        function_flags |= SRFLAG_FN_TYPE_EXTERN;
    }

    if (set.peek_next_eq(TokenType::KEYWORD_ASYNC))
    {
        set.next();
        function_flags |= SRFLAG_FN_TYPE_ASYNC;
    }

    auto reference_token = set.expect(TokenType::KEYWORD_FN);

    // Here we expect to receive the function name
    const auto fn_name_tok = set.expect(TokenType::IDENTIFIER, "Expected function name");
    const auto& fn_name = fn_name_tok.get_lexeme();

    auto function_context = std::make_shared<ParsingContext>(context, ContextType::FUNCTION);

    GenericParameterList generic_parameter_names = parse_generic_declaration(set);

    if (function_flags & SRFLAG_FN_TYPE_EXTERN && !generic_parameter_names.empty())
    {
        set.throw_error("Extern functions cannot have generic parameters");
    }

    set.expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters;

    // Parameter parsing
    if (!set.peek_next_eq(TokenType::RPAREN))
    {
        parse_function_parameters(function_context, set, parameters, function_flags);

        if (!set.peek_next_eq(TokenType::RPAREN))
        {
            set.throw_error(
                "Expected closing parenthesis after variadic parameter; variadic parameter must be the last parameter in the function signature"
            );
        }
    }

    set.expect(TokenType::RPAREN, "Expected ')' after function parameters");
    set.expect(TokenType::COLON, "Expected a colon after function definition");

    // Return type doesn't have the same flags as the function, hence NONE
    auto return_type = parse_type(context, set, { "Expected return type in function header" });

    const auto& position = reference_token.get_source_fragment();
    auto sym_function_name = Symbol(position, context->get_name(), fn_name);

    std::unique_ptr<AstBlock> body = nullptr;

    if (function_flags & SRFLAG_FN_TYPE_EXTERN)
    {
        set.expect(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
        body = AstBlock::create_empty(function_context, position);
    }
    else
    {
        body = parse_block(function_context, set);
    }

    return std::make_unique<AstFunctionDeclaration>(
        function_context,
        sym_function_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        modifier,
        function_flags,
        std::move(generic_parameter_names)
    );
}

std::unique_ptr<AstBlock> consume_anonymous_fn_body(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    if (!set.peek_next_eq(TokenType::LBRACE))
    {
        auto expr = parse_inline_expression(context, set);

        const auto src_frag = expr->get_source_fragment();
        std::vector<std::unique_ptr<IAstNode>> body_nodes;
        body_nodes.push_back(std::move(expr));

        return std::make_unique<AstBlock>(
            src_frag,
            context,
            std::move(body_nodes)
        );
    }

    return parse_block(context, set);
}

std::unique_ptr<IAstExpression> stride::ast::parse_anonymous_fn_expression(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    int function_flags = SRFLAG_FN_TYPE_ANONYMOUS;
    auto function_context = std::make_shared<ParsingContext>(
        context,
        ContextType::FUNCTION
    );

    // Parses expressions like:
    // (<param1>: <type1>, ...): <ret_type> -> {}
    if (auto header_definition = collect_parenthesized_block(set);
        header_definition.has_value() && header_definition->has_next())
    {
        parse_function_parameters(
            function_context,
            header_definition.value(),
            parameters,
            function_flags
        );
    }

    set.expect(TokenType::COLON, "Expected ':' after lambda function header definition");
    auto return_type = parse_type(
        function_context,
        set,
        { "Expected type after anonymous function header definition" }
    );
    const auto lambda_arrow = set.expect(
        TokenType::RARROW,
        "Expected '->' after lambda parameters"
    );

    auto lambda_body = consume_anonymous_fn_body(function_context, set);

    static int anonymous_lambda_id = 0;

    auto symbol_name = Symbol(
        { set.get_source(),
          reference_token.get_source_fragment().offset,
          lambda_arrow.get_source_fragment().offset -
          reference_token.get_source_fragment().offset },
        ANONYMOUS_FN_PREFIX + std::to_string(anonymous_lambda_id++)
    );

    std::vector<std::unique_ptr<IAstType>> cloned_params;
    cloned_params.reserve(parameters.size());
    for (auto& param : parameters)
    {
        cloned_params.push_back(param->get_type()->clone_ty());
    }

    return std::make_unique<AstLambdaFunctionExpression>(
        function_context,
        symbol_name,
        std::move(parameters),
        std::move(lambda_body),
        std::move(return_type),
        VisibilityModifier::PRIVATE,
        // Anonymous functions are always private
        function_flags
    );
}

std::vector<AstReturnStatement*> collect_return_statements(const AstBlock* body)
{
    if (!body)
    {
        return {};
    }

    std::vector<AstReturnStatement*> return_statements;
    for (const auto& child : body->get_children())
    {
        if (auto* return_stmt = dynamic_cast<AstReturnStatement*>(child.get()))
        {
            return_statements.push_back(return_stmt);
        }

        // Recursively collect from child containers
        if (const auto container_node = dynamic_cast<IAstContainer*>(child.
            get()))
        {
            const auto aggregated = collect_return_statements(
                container_node->get_body());
            return_statements.insert(return_statements.end(),
                                     aggregated.begin(),
                                     aggregated.end());
        }

        // Edge case: if statements hold the `else` block too, though this doesn't fall under the
        // `IAstContainer` abstraction. The `get_body` part is added in the previous case, though we
        // still need to add the else body
        if (const auto if_statement = dynamic_cast<AstConditionalStatement*>(child.
            get()))
        {
            const auto aggregated = collect_return_statements(
                if_statement->get_else_body());
            return_statements.insert(
                return_statements.end(),
                aggregated.begin(),
                aggregated.end()
            );
        }
    }
    return return_statements;
}

void collect_free_variables(
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

std::vector<std::unique_ptr<AstFunctionParameter>> IAstFunction::get_parameters() const
{
    std::vector<std::unique_ptr<AstFunctionParameter>> cloned_params;
    cloned_params.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        cloned_params.push_back(param->clone_as<AstFunctionParameter>());
    }

    return cloned_params;
}

void IAstFunction::validate_candidate(IAstFunction* candidate)
{
    candidate->get_body()->validate();

    const auto& ret_ty = candidate->get_return_type();
    const auto return_statements = collect_return_statements(candidate->get_body());

    // For void types, we only disallow returning expressions, as this is redundant.
    if (const auto void_ret = cast_type<AstPrimitiveType*>(ret_ty);
        void_ret != nullptr && void_ret->get_primitive_type() == PrimitiveType::VOID)
    {
        for (const auto& return_stmt : return_statements)
        {
            if (return_stmt->get_return_expression().has_value())
            {
                throw stride::parsing_error(
                    stride::ErrorType::TYPE_ERROR,
                    std::format(
                        "{} has return type 'void' and cannot return a value.",
                        candidate->is_anonymous()
                        ? "Anonymous function"
                        : std::format("Function '{}'", candidate->get_function_name())),
                    {
                        stride::ErrorSourceReference(
                            "unexpected return value",
                            return_stmt->get_source_fragment()
                        ),
                        stride::ErrorSourceReference(
                            "Function returning void type",
                            candidate->get_source_fragment()
                        )
                    }

                );
            }
        }
        return;
    }

    if (return_statements.empty())
    {
        if (cast_type<AstAliasType*>(ret_ty))
        {
            throw stride::parsing_error(
                stride::ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' returns a struct type, but no return statement is present.",
                    candidate->get_function_name()),
                candidate->get_source_fragment());
        }

        throw stride::parsing_error(
            stride::ErrorType::COMPILATION_ERROR,
            std::format(
                "{} is missing a return statement.",
                candidate->is_anonymous()
                ? "Anonymous function"
                : std::format("Function '{}'", candidate->get_function_name())),
            candidate->get_source_fragment()
        );
    }

    for (const auto& return_stmt : return_statements)
    {
        if (return_stmt->is_void_type())
        {
            if (!ret_ty->is_void_ty())
            {
                throw stride::parsing_error(
                    stride::ErrorType::TYPE_ERROR,
                    std::format(
                        "Function '{}' returns a value of type '{}', but no return statement is present.",
                        candidate->is_anonymous() ? "<anonymous function>" : candidate->get_function_name(),
                        ret_ty->to_string()),
                    return_stmt->get_source_fragment()
                );
            }
            return;
        }

        if (const auto& ret_expr = return_stmt->get_return_expression().value();
            !ret_expr->get_type()->equals(ret_ty) &&
            !ret_expr->get_type()->is_assignable_to(ret_ty))
        {
            const auto error_fragment = stride::ErrorSourceReference(
                std::format(
                    "expected {}{}",
                    candidate->get_return_type()->is_primitive()
                    ? ""
                    : candidate->get_return_type()->is_function()
                    ? "function-type "
                    : "struct-type ",
                    candidate->get_return_type()->to_string()),
                ret_expr->get_source_fragment()
            );

            throw stride::parsing_error(
                stride::ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' expected a return type of '{}', but received '{}'.",
                    candidate->is_anonymous() ? "<anonymous function>" : candidate->get_function_name(),
                    ret_ty->get_type_name(),
                    ret_expr->get_type()->get_type_name()),
                { error_fragment }
            );
        }
    }
}

void IAstFunction::validate()
{
    // Extern functions have no body to validate
    if (this->is_extern())
        return;

    std::vector<std::unique_ptr<IAstFunction>> validatable_candidates;

    //
    // For generic functions, we create a new copy of the function with all parameters resolved, and do validation
    // on that copy. This is because we want to validate the function body with the actual types that will be used in
    // the function, rather than the generic placeholders.
    //
    // create a copy of this function with the parameters instantiated
    for (const auto definition = this->get_function_definition();
         const auto& [types, function, node] : definition->get_instantiations())
    {
        auto instantiated_return_ty = resolve_generics(
            this->_annotated_return_type.get(),
            this->_generic_parameters,
            types
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
                    resolve_generics(param->get_type(), this->_generic_parameters, types)
                )
            );
        }

        const auto& candidate = std::make_unique<AstFunctionDeclaration>(
            this->get_context(),
            this->_symbol,
            std::move(instantiated_function_params),
            this->_body->clone_as<AstBlock>(),
            std::move(instantiated_return_ty),
            this->get_visibility(),
            this->_flags,
            this->get_generic_parameters()
        );

        validate_candidate(candidate.get());
    }
}

llvm::Value* IAstFunction::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Function* function = nullptr;

    for (const auto& [function_name, llvm_function_val] : this->get_function_overload_metadata())
    {
        if (!llvm_function_val)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Function symbol '{}' missing", function_name),
                this->get_source_fragment()
            );
        }


        // If the function body has already been generated (has basic blocks), just return the function pointer
        if (this->is_extern() || !llvm_function_val->empty())
        {
            return llvm_function_val;
        }

        // Save the current insert point to restore it later
        // This is important when generating nested lambdas
        llvm::BasicBlock* saved_insert_block = builder->GetInsertBlock();
        llvm::BasicBlock::iterator saved_insert_point;
        const bool has_insert_point = saved_insert_block != nullptr;

        if (saved_insert_block)
        {
            saved_insert_point = builder->GetInsertPoint();
        }

        llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(
            module->getContext(),
            "entry",
            llvm_function_val
        );
        builder->SetInsertPoint(entry_bb);

        // We create a new builder for the prologue to ensure allocas are at the very top
        llvm::IRBuilder prologue_builder(
            &llvm_function_val->getEntryBlock(),
            llvm_function_val->getEntryBlock().begin()
        );

        //
        // Captured variable handling
        // Map captured variables to function arguments with __capture_ prefix
        //
        auto fn_parameter_argument = llvm_function_val->arg_begin();
        for (const auto& capture : this->_captured_variables)
        {
            if (fn_parameter_argument != llvm_function_val->arg_end())
            {
                fn_parameter_argument->setName(closures::format_captured_variable_name(capture.internal_name));

                // Create alloca with __capture_ prefix so identifier lookup can find it
                llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                    fn_parameter_argument->getType(),
                    nullptr,
                    closures::format_captured_variable_name_internal(capture.internal_name)
                );

                builder->CreateStore(fn_parameter_argument, alloca);
                ++fn_parameter_argument;
            }
        }

        //
        // Function parameter handling
        // Here we define the parameters on the stack as memory slots for the function
        //
        for (const auto& param : this->_parameters)
        {
            if (fn_parameter_argument != llvm_function_val->arg_end())
            {
                fn_parameter_argument->setName(param->get_name() + ".arg");

                // Create a memory slot on the stack for the parameter
                llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                    fn_parameter_argument->getType(),
                    nullptr,
                    param->get_name()
                );

                // Store the initial argument value into the alloca
                builder->CreateStore(fn_parameter_argument, alloca);

                ++fn_parameter_argument;
            }
        }

        // Generate Body
        llvm::Value* function_body_value = this->_body->codegen(module, builder);

        // Final Safety: Implicit Return
        // If the get_body didn't explicitly return (no terminator found), add one.
        if (llvm::BasicBlock* current_bb = builder->GetInsertBlock();
            current_bb && !current_bb->getTerminator())
        {
            if (llvm::Type* ret_type = llvm_function_val->getReturnType();
                ret_type->isVoidTy())
            {
                builder->CreateRetVoid();
            }
            else if (function_body_value && function_body_value->getType() == ret_type)
            {
                builder->CreateRet(function_body_value);
            }
            // Default return to keep IR valid (useful for main or incomplete functions)
            else
            {
                if (ret_type->isFloatingPointTy())
                {
                    builder->CreateRet(llvm::ConstantFP::get(ret_type, 0.0));
                }
                else if (ret_type->isIntegerTy())
                {
                    builder->CreateRet(llvm::ConstantInt::get(ret_type, 0));
                }
                else
                {
                    throw parsing_error(
                        ErrorType::COMPILATION_ERROR,
                        std::format(
                            "Function '{}' is missing a return path.",
                            this->get_function_name()
                        ),
                        this->get_source_fragment()
                    );
                }
            }
        }

        if (llvm::verifyFunction(*llvm_function_val, &llvm::errs()))
        {
            module->print(llvm::errs(), nullptr);
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "LLVM Function Verification Failed for: " + this->get_function_name(),
                this->get_source_fragment()
            );
        }

        // Restore the previous insert point for nested lambda generation
        if (has_insert_point && saved_insert_block)
        {
            builder->SetInsertPoint(saved_insert_block, saved_insert_point);
        }

        // For anonymous lambdas with captured variables, create a closure structure
        // that bundles the function pointer with the current values of captured variables
        if (this->is_anonymous())
        {
            // Collect the current values of captured variables from the enclosing scope
            std::vector<llvm::Value*> captured_values;
            for (const auto& capture : this->get_captured_variables())
            {
                if (const auto block = builder->GetInsertBlock())
                {
                    llvm::Function* current_fn = block->getParent();
                    llvm::Value* captured_val = closures::lookup_variable_or_capture(current_fn, capture.internal_name);

                    if (!captured_val)
                    {
                        captured_val = closures::lookup_variable_by_base_name(current_fn, capture.name);
                    }

                    if (captured_val)
                    {
                        // Load the value if it's an alloca
                        if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(captured_val))
                        {
                            captured_val = builder->CreateLoad(
                                alloca->getAllocatedType(),
                                alloca,
                                capture.internal_name
                            );
                        }
                        captured_values.push_back(captured_val);
                    }
                }
            }

            // Create and return a closure instead of the raw function pointer
            return closures::create_closure(module, builder, llvm_function_val, captured_values);
        }

        function = llvm_function_val;
    }

    return function;
}

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

    const auto linkage = this->_visibility == VisibilityModifier::PRIVATE
        ? llvm::Function::PrivateLinkage
        : llvm::Function::ExternalLinkage;

    const auto& definition = this->get_function_definition();

    if (definition->get_instantiations().empty())
    {
        std::cerr << "Warning: No instantiations found for function '" << this->get_function_name() <<
            "'. This function will not be emitted in the LLVM IR.\n";
        return;
    }
    for (const auto& [types, instantiation_fn, node] : definition->get_instantiations())
    {
        const auto overloaded_fn_name = get_overloaded_function_name(node->get_internalized_function_name(), types);
        llvm::FunctionType* generic_function_type = node->get_generic_instantiated_llvm_function_type(
            module,
            captured_types,
            types
        );

        instantiation_fn = llvm::Function::Create(
            generic_function_type,
            linkage,
            overloaded_fn_name,
            module
        );

        if (node->is_anonymous())
            instantiation_fn->addFnAttr("stride.anonymous");

        this->_body->resolve_forward_references(module, builder);
    }
}

llvm::FunctionType* IAstFunction::get_generic_instantiated_llvm_function_type(
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
            base_parameter_types.push_back(resolved_generic_param_type->get_llvm_type(module));
        }
    }

    std::vector<llvm::Type*> parameter_types;
    parameter_types.reserve(base_parameter_types.size() + captured_variables.size());

    // Captured variables are first in the internal LLVM function type
    parameter_types.insert(parameter_types.end(), captured_variables.begin(), captured_variables.end());
    parameter_types.insert(parameter_types.end(), base_parameter_types.begin(), base_parameter_types.end());

    const auto return_type = this->get_return_type()->get_llvm_type(module);

    if (!return_type)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Could not get LLVM return type for function: " + this->get_function_name(),
            this->get_source_fragment()
        );
    }

    const auto& llvm_function_ty = llvm::FunctionType::get(return_type, parameter_types, this->is_variadic());;

    if (const auto fn = module->getFunction(this->get_internalized_function_name());
        fn != nullptr && fn->getFunctionType() != llvm_function_ty)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "Function symbol '{}' already exists with a different signature",
                this->get_internalized_function_name()
            ),
            this->get_source_fragment()
        );
    }

    return llvm_function_ty;
}

std::vector<std::unique_ptr<IAstType>> IAstFunction::get_parameter_types() const
{
    std::vector<std::unique_ptr<IAstType>> types;
    types.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        types.push_back(param->get_type()->clone_ty());
    }

    return types;
}

FunctionDefinition* IAstFunction::get_function_definition()
{
    if (this->_function_definition != nullptr)
        return this->_function_definition;

    const auto& definition = this->get_context()->get_function_definition(
        this->get_function_name(),
        this->get_parameter_types(),
        this->get_generic_parameters().size()
    );

    if (!definition.has_value())
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Function definition for '{}' not found in context", this->get_internalized_function_name()),
            this->get_source_fragment()
        );
    }

    this->_function_definition = definition.value();
    return this->_function_definition;
}

std::vector<GenericFunctionMetadata> IAstFunction::get_function_overload_metadata()
{
    const auto& definition = this->get_function_definition();

    // If the function is not generic, we just return a singular name (the regular internalized name)
    if (!definition->get_type()->is_generic())
    {
        return {
            GenericFunctionMetadata{ this->get_internalized_function_name(), definition->get_llvm_function() }
        };
    }

    std::vector<GenericFunctionMetadata> metadata;

    for (const auto& [types, llvm_function, node] : definition->get_instantiations())
    {
        metadata.emplace_back(
            get_overloaded_function_name(node->get_internalized_function_name(), types),
            llvm_function
        );
    }

    return metadata;
}

std::unique_ptr<IAstNode> AstFunctionParameter::clone()
{
    return std::make_unique<AstFunctionParameter>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_name(),
        this->get_type()->clone_ty()
    );
}

std::unique_ptr<IAstNode> IAstFunction::clone()
{
    std::vector<std::unique_ptr<AstFunctionParameter>> cloned_params;
    cloned_params.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        cloned_params.push_back(param->clone_as<AstFunctionParameter>());
    }

    return std::make_unique<IAstFunction>(
        this->get_source_fragment(),
        this->get_context(),
        this->_symbol,
        std::move(cloned_params),
        this->_body->clone_as<AstBlock>(),
        this->_annotated_return_type->clone_ty(),
        this->_visibility,
        this->_flags,
        this->_generic_parameters
    );
}

std::string IAstFunction::to_string()
{
    std::string params;
    for (const auto& param : this->_parameters)
    {
        if (!params.empty())
            params += ", ";
        params += param->to_string();
    }

    const auto body_str = this->get_body() == nullptr
        ? "<empty>"
        : this->get_body()->to_string();

    return std::format(
        "Function(name: {}(internal: {}), params: [{}], body: {}{} -> {})",
        this->is_anonymous() ? "<anonymous>" : this->get_function_name(),
        this->get_internalized_function_name(),
        params,
        body_str,
        this->is_extern() ? " (extern)" : "",
        this->get_return_type()->to_string()
    );
}
