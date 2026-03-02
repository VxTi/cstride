#include "ast/nodes/function_declaration.h"

#include "errors.h"
#include "ast/casting.h"
#include "ast/closures.h"
#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/while_loop.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

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
    [[maybe_unused]] VisibilityModifier modifier
)
{
    int function_flags = 0;
    if (set.peek_next_eq(TokenType::KEYWORD_EXTERN))
    {
        set.next();
        function_flags |= SRFLAG_FN_DEF_EXTERN;
    }

    if (set.peek_next_eq(TokenType::KEYWORD_ASYNC))
    {
        set.next();
        function_flags |= SRFLAG_FN_DEF_ASYNC;
    }

    auto reference_token = set.expect(TokenType::KEYWORD_FN);

    // Here we expect to receive the function name
    const auto fn_name_tok = set.expect(TokenType::IDENTIFIER,
                                        "Expected function name after 'fn'");
    const auto& fn_name = fn_name_tok.get_lexeme();

    auto function_context = std::make_shared<ParsingContext>(context, ContextType::FUNCTION);

    set.expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

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
    auto return_type = parse_type(context, set, "Expected return type in function header");

    const auto& position = reference_token.get_source_fragment();
    auto sym_function_name = Symbol(position, context->get_name(), fn_name);

    // Prevent tagging extern functions with different internal names.
    // This prevents the linker from being unable to make a reference to this function.
    if ((function_flags & SRFLAG_FN_DEF_EXTERN) == 0)
    {
        std::vector<IAstType*> parameter_types;
        parameter_types.reserve(parameters.size());

        for (const auto& param : parameters)
        {
            parameter_types.push_back(param->get_type());
        }
        sym_function_name = resolve_internal_function_name(
            context,
            position,
            { fn_name },
            parameter_types
        );
    }

    std::unique_ptr<AstBlock> body = nullptr;

    if (function_flags & SRFLAG_FN_DEF_EXTERN)
    {
        set.expect(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
        body = AstBlock::create_empty(function_context, position);
    }
    else
    {
        body = parse_block(function_context, set);
    }

    auto decl = std::make_unique<AstFunctionDeclaration>(
        function_context,
        sym_function_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        function_flags
    );

    // Register the function's type in the context immediately after parsing so that
    // forward references and out-of-order calls are resolvable during type inference.
    std::vector<std::unique_ptr<IAstType>> param_types;
    param_types.reserve(decl->get_parameters_ref().size());
    for (const auto& param : decl->get_parameters_ref())
        param_types.push_back(param->get_type()->clone_ty());

    context->define_function(
        sym_function_name,
        std::make_unique<AstFunctionType>(
            sym_function_name.symbol_position,
            context,
            std::move(param_types),
            decl->get_return_type()->clone_ty()
        )
    );

    return decl;
}

void stride::ast::parse_function_parameters(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters,
    int& function_flags
)
{
    // If the first argument is variadic, no other parameters are allowed.
    if (set.peek_next_eq(TokenType::THREE_DOTS))
    {
        function_flags |= SRFLAG_FN_DEF_VARIADIC;
        set.next();
        return;
    }

    parse_standalone_fn_param(context, set, parameters);

    int recursion_depth = 0;
    while (set.peek_next_eq(TokenType::COMMA))
    {
        set.next(); // Skip comma
        const auto next = set.peek_next();

        if (parameters.size() > MAX_FUNCTION_PARAMETERS)
        {
            throw parsing_error(
                ErrorType::SYNTAX_ERROR,
                std::format("Function cannot have more than {} parameters",
                            MAX_FUNCTION_PARAMETERS),
                next.get_source_fragment()
            );
        }

        // Variadic arguments are noted like "..." and must be the last parameter,
        // so if we encounter it, we can break out of the loop immediately.
        if (next.get_type() == TokenType::THREE_DOTS)
        {
            function_flags |= SRFLAG_FN_DEF_VARIADIC;
            set.next();
            break;
        }

        parse_standalone_fn_param(context, set, parameters);

        if (recursion_depth++ > MAX_RECURSION_DEPTH)
        {
            set.throw_error(
                "Maximum recursion depth exceeded when parsing function parameters"
            );
        }
    }
}

void stride::ast::parse_standalone_fn_param(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    int flags = 0;

    if (set.peek_next_eq(TokenType::KEYWORD_LET))
    {
        flags |= SRFLAG_FN_PARAM_DEF_MUTABLE;
        set.next();
    }

    const auto reference_token =
        set.expect(TokenType::IDENTIFIER, "Expected a function parameter name");
    set.expect(TokenType::COLON);

    std::unique_ptr<IAstType> fn_param_type =
        parse_type(context, set, "Expected function parameter type", flags);

    const auto& param_name = reference_token.get_lexeme();

    if (std::ranges::find_if(
        parameters,
        [&](const std::unique_ptr<AstFunctionParameter>& p)
        {
            return p->get_name() == param_name;
        }) != parameters.end())
    {
        TokenSet::throw_error(
            reference_token,
            ErrorType::SEMANTIC_ERROR,
            std::format(
                "Duplicate parameter name '{}' in function definition",
                param_name
            )
        );
    }

    parameters.push_back(std::make_unique<AstFunctionParameter>(
        reference_token.get_source_fragment(),
        context,
        param_name,
        std::move(fn_param_type)
    ));
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
    if (const auto* identifier = dynamic_cast<const AstIdentifier*>(node))
    {
        capture_variable(identifier->get_name());
        return;
    }

    // Handle nested callables (lambdas) - recursively collect their free variables
    if (auto* callable = dynamic_cast<IAstFunction*>(node))
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
                        var_def->get_type()->clone_ty());
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
        if (return_stmt->get_return_expr())
        {
            collect_free_variables(
                return_stmt->get_return_expr(),
                lambda_context,
                outer_context,
                captures);
        }
        return;
    }

    // Handle function calls
    if (const auto* fn_call = cast_expr<AstFunctionCall*>(node))
    {
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
                var_decl->get_initial_value().get(),
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
    if (const auto* struct_init = cast_expr<AstStructInitializer*>(node))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
        {
            collect_free_variables(val.get(), lambda_context, outer_context, captures);
        }
        return;
    }

    // Handle member access
    if (const auto* member = cast_expr<AstMemberAccessor*>(node))
    {
        collect_free_variables(member->get_base(), lambda_context, outer_context, captures);
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

void IAstFunction::validate()
{
    if (this->is_anonymous())
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
                this->get_context()->define_variable(capture, outer_var->get_type()->clone_ty());
            }
        }
    }

    // Extern functions don't require return statements and have no function body, so no validation
    // needed.
    if (this->is_extern())
    {
        return;
    }

    if (this->get_body() != nullptr)
    {
        this->get_body()->validate();
    }


    const auto return_statements = collect_return_statements(this->get_body());

    // For void types, we only disallow returning expressions, as this is redundant.
    if (const auto void_ret = cast_type<AstPrimitiveType*>(
            this->get_return_type());
        void_ret != nullptr && void_ret->get_type() == PrimitiveType::VOID)
    {
        for (const auto& return_stmt : return_statements)
        {
            if (return_stmt->get_return_expr() != nullptr)
            {
                throw parsing_error(
                    ErrorType::TYPE_ERROR,
                    std::format(
                        "Function '{}' has return type 'void' and cannot return a value.",
                        this->get_name()),
                    return_stmt->get_source_fragment());
            }
        }
        return;
    }

    if (return_statements.empty())
    {
        if (cast_type<AstNamedType*>(this->get_return_type()))
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' returns a struct type, but no return statement is present.",
                    this->get_name()),
                this->get_source_fragment());
        }

        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "Function '{}' is missing a return statement.",
                this->is_anonymous()
                ? "<anonymous function>"
                : this->get_name()),
            this->get_source_fragment());
    }

    for (const auto& return_stmt : return_statements)
    {
        const auto ret_expr = return_stmt->get_return_expr();

        if (ret_expr == nullptr)
        {
            continue;
        }

        if (!ret_expr->get_type()->equals(*this->get_return_type()))
        {
            const auto error_fragment = ErrorSourceReference(
                std::format(
                    "expected {}{}",
                    this->get_return_type()->is_primitive()
                    ? ""
                    : this->get_return_type()->is_function()
                    ? "function-type "
                    : "struct-type ",
                    this->get_return_type()->to_string()),
                return_stmt->get_return_expr()->get_source_fragment()
            );

            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' expected a return type of '{}', but received '{}'.",
                    this->is_anonymous() ? "<anonymous function>" : this->get_name(),
                    this->get_return_type()->to_string(),
                    ret_expr->get_type()->to_string()),
                { error_fragment }
            );
        }
    }

    // We'll have to validate whether it:
    // 1. Requires a return AST node - This can be the case when the return type is not a primitive,
    // e.g., a struct
    // 2. The return type doesn't match the function signature
    // 3. All code paths return a value (if not void)
}


llvm::Value* IAstFunction::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Function* function = module->getFunction(this->get_internal_name());
    if (!function)
    {
        module->print(llvm::errs(), nullptr);
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Function symbol missing: " + this->get_internal_name(),
            this->get_source_fragment()
        );
    }

    if (this->is_extern())
    {
        return function;
    }

    // If the function body has already been generated (has basic blocks), just return the function pointer
    if (!function->empty())
    {
        return function;
    }

    // Save the current insert point to restore it later
    // This is important when generating nested lambdas
    llvm::BasicBlock* saved_insert_block = builder->GetInsertBlock();
    llvm::BasicBlock::iterator saved_insert_point;
    bool has_insert_point = false;
    if (saved_insert_block)
    {
        saved_insert_point = builder->GetInsertPoint();
        has_insert_point = true;
    }

    llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(
        module->getContext(),
        "entry",
        function
    );
    builder->SetInsertPoint(entry_bb);

    // We create a new builder for the prologue to ensure allocas are at the very top
    llvm::IRBuilder prologue_builder(&function->getEntryBlock(), function->getEntryBlock().begin());

    //
    // Captured variable handling
    // Map captured variables to function arguments with __capture_ prefix
    //
    auto arg_it = function->arg_begin();
    for (const auto& capture : this->get_captured_variables())
    {
        if (arg_it != function->arg_end())
        {
            arg_it->setName(closures::format_captured_variable_name(capture.internal_name));

            // Create alloca with __capture_ prefix so identifier lookup can find it
            llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                arg_it->getType(),
                nullptr,
                closures::format_captured_variable_name_internal(capture.internal_name)
            );

            builder->CreateStore(arg_it, alloca);
            ++arg_it;
        }
    }

    //
    // Function parameter handling
    // Here we define the parameters on the stack as memory slots for the function
    //
    for (const auto& param : this->_parameters)
    {
        if (arg_it != function->arg_end())
        {
            arg_it->setName(param->get_name() + ".arg");

            // Create a memory slot on the stack for the parameter
            llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                arg_it->getType(),
                nullptr,
                param->get_name()
            );

            // Store the initial argument value into the alloca
            builder->CreateStore(arg_it, alloca);

            ++arg_it;
        }
    }

    // Generate Body
    llvm::Value* last_val = nullptr;
    if (this->get_body())
    {
        last_val = this->get_body()->codegen(module, builder);
    }

    // Final Safety: Implicit Return
    // If the get_body didn't explicitly return (no terminator found), add one.
    if (llvm::BasicBlock* current_bb = builder->GetInsertBlock();
        current_bb && !current_bb->getTerminator())
    {
        if (llvm::Type* ret_type = function->getReturnType(); ret_type->
            isVoidTy())
        {
            builder->CreateRetVoid();
        }
        else if (last_val && last_val->getType() == ret_type)
        {
            builder->CreateRet(last_val);
        }
        else
        {
            // Default return to keep IR valid (useful for main or incomplete functions)
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
                    "Function " + this->get_name() + " missing return path.",
                    this->get_source_fragment()
                );
            }
        }
    }

    if (llvm::verifyFunction(*function, &llvm::errs()))
    {
        function->print(llvm::errs(), nullptr);
        // Print IR to console to see what's wrong
        throw std::runtime_error(
            "LLVM Function Verification Failed for: " + this->get_name());
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
                llvm::Value* captured_val = closures::lookup_variable_or_capture(
                    current_fn,
                    capture.internal_name);

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
        return closures::create_closure(module, builder, function, captured_values);
    }

    return function;
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
        this->_return_type->clone_ty(),
        this->_flags
    );
}

std::optional<std::vector<llvm::Type*>> AstFunctionDeclaration::resolve_parameter_types(
    llvm::Module* module
) const
{
    std::vector<llvm::Type*> param_types;
    for (const auto& param : this->_parameters)
    {
        auto llvm_type = type_to_llvm_type(param->get_type(), module);
        if (!llvm_type)
        {
            return std::nullopt;
        }
        param_types.push_back(llvm_type);
    }
    return param_types;
}

std::unique_ptr<IAstExpression> stride::ast::parse_lambda_fn_expression(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    int function_flags = SRFLAG_FN_DEF_ANONYMOUS;
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
        "Expected type after anonymous function header definition"
    );
    const auto lambda_arrow = set.expect(
        TokenType::DASH_RARROW,
        "Expected '->' after lambda parameters"
    );

    auto lambda_body = parse_block(function_context, set);

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
        context,
        symbol_name,
        std::move(parameters),
        std::move(lambda_body),
        std::move(return_type),
        function_flags
    );
}

bool stride::ast::is_lambda_fn_expression(const TokenSet& set)
{
    return set.peek_eq(TokenType::LPAREN, 0)
        && set.peek_eq(TokenType::IDENTIFIER, 1)
        && set.peek_eq(TokenType::COLON, 2);
}

void IAstFunction::resolve_forward_references(
    ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    const auto& function_name = this->get_internal_name();

    // Avoid re-registering if already declared (e.g. called multiple times)
    if (module->getFunction(function_name))
        return;

    llvm::Type* return_type = type_to_llvm_type(
        this->get_return_type(),
        module
    );
    if (!return_type)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Failed to resolve return type for function '{}'", this->get_name()),
            this->get_return_type()->get_source_fragment()
        );
    }

    std::vector<llvm::Type*> param_types;
    param_types.reserve(_captured_variables.size() + _parameters.size());

    // Add captured variables as first parameters
    for (const auto& capture : this->_captured_variables)
    {
        if (const auto capture_def = this->get_context()->lookup_variable(capture.name, true))
        {
            if (llvm::Type* capture_type = type_to_llvm_type(
                capture_def->get_type(),
                module))
            {
                param_types.push_back(capture_type);
            }
        }
    }

    // Add regular parameters
    for (const auto& param : this->_parameters)
    {
        llvm::Type* llvm_type = type_to_llvm_type(param->get_type(), module);
        if (!llvm_type)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Failed to resolve parameter type for function '{}'", function_name),
                param->get_type()->get_source_fragment()
            );
        }

        param_types.push_back(llvm_type);
    }

    llvm::FunctionType* function_type = llvm::FunctionType::get(
        return_type,
        param_types,
        this->is_variadic()
    );

    const auto linkage = this->is_anonymous()
        ? llvm::Function::PrivateLinkage
        : llvm::Function::ExternalLinkage;

    llvm::Function::Create(
        function_type,
        linkage,
        function_name,
        module
    );

    // Recursively resolve forward references in the function body
    // This is necessary for nested lambdas (e.g., lambdas that return lambdas)
    if (this->get_body())
    {
        this->get_body()->resolve_forward_references(context, module, builder);
    }
}

std::string AstFunctionDeclaration::to_string()
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
        "FunctionDeclaration(name: {}(internal: {}), params: [{}], body: {}{} -> {})",
        this->get_name(),
        this->get_internal_name(),
        params,
        body_str,
        this->is_extern() ? " (extern)" : "",
        this->get_return_type()->to_string());
}

std::string AstLambdaFunctionExpression::to_string()
{
    return "LambdaFunction";
}

std::string AstFunctionParameter::to_string()
{
    const auto name = this->get_name();
    auto type_str = this->get_type()->to_string();
    return std::format("{}({})", name, type_str);
}
