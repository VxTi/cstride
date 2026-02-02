#include "ast/nodes/function_declaration.h"

#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include "ast/internal_names.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/if_statement.h"
#include "ast/nodes/return.h"


using namespace stride::ast;

std::vector<AstReturn*> collect_return_statements(
    const AstBlock* body
)
{
    if (!body) return {};

    std::vector<AstReturn*> return_statements;
    for (const auto& child : body->children())
    {
        if (auto* return_stmt = dynamic_cast<AstReturn*>(child.get()))
        {
            return_statements.push_back(return_stmt);
        }

        // Recursively collect from child containers
        if (const auto container_node = dynamic_cast<IAstContainer*>(child.get()))
        {
            const auto aggregated = collect_return_statements(container_node->get_body());
            return_statements.insert(return_statements.end(), aggregated.begin(), aggregated.end());
        }

        // Edge case: if statements hold the `else` block too, though this doesn't fall under the `IAstContainer` abstraction.
        // The `get_body` part is added in the previous case, though we still need to add the else body
        if (const auto if_statement = dynamic_cast<AstIfStatement*>(child.get()))
        {
            const auto aggregated = collect_return_statements(if_statement->get_else_body());
            return_statements.insert(return_statements.end(), aggregated.begin(), aggregated.end());
        }
    }
    return return_statements;
}

void AstFunctionDeclaration::validate()
{
    if (this->get_body() != nullptr)
    {
        this->get_body()->validate();
    }

    const auto return_statements = collect_return_statements(this->get_body());

    // For void types, we only disallow returning expressions, as this is redundant.
    if (const auto void_ret = dynamic_cast<AstPrimitiveType*>(this->get_return_type());
        void_ret != nullptr && void_ret->get_type() == PrimitiveType::VOID)
    {
        for (const auto& return_stmt : return_statements)
        {
            if (return_stmt->get_return_expr() != nullptr)
            {
                throw parsing_error(
                    ErrorType::TYPE_ERROR,
                    std::format("Function '{}' has return get_type 'void' and cannot return a value.",
                                this->get_name()),
                    *this->get_source(),
                    return_stmt->get_source_position()
                );
            }
        }
        return;
    }

    if (return_statements.empty())
    {
        if (dynamic_cast<AstStructType*>(this->get_return_type()))
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Function '{}' returns a struct get_type, but no return statement is present.",
                            this->get_name()),
                *this->get_source(),
                this->get_source_position()
            );
        }

        throw parsing_error(
            ErrorType::RUNTIME_ERROR,
            std::format("Function '{}' is missing a return statement.", this->get_name()),
            *this->get_source(),
            this->get_source_position()
        );
    }

    for (const auto& return_stmt : return_statements)
    {
        if (const auto return_type = infer_expression_type(this->get_registry(), return_stmt->get_return_expr());
            !return_type->equals(*this->get_return_type()))
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Function '{}' expected a return get_type of '{}', but received '{}'.",
                    this->get_name(), this->get_return_type()->to_string(), return_type->to_string()
                ),
                {
                    ErrorSourceReference(
                        std::format("expected {}", this->get_return_type()->to_string()),
                        *this->get_source(),
                        return_stmt->get_return_expr()->get_source_position()
                    )
                }
            );
        }
    }

    // We'll have to validate whether it:
    // 1. Requires a return AST node - This can be the case when the return get_type is not a primitive, e.g., a struct
    // 2. The return get_type doesn't match the function signature
    // 3. All code paths return a value (if not void)
}

void AstFunctionDeclaration::resolve_forward_references(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    const auto fn_name = this->get_internal_name();

    const std::optional<std::vector<llvm::Type*>> param_types = resolve_parameter_types(module);
    llvm::Type* return_type = internal_type_to_llvm_type(this->get_return_type(), module);

    if (!param_types || !return_type)
    {
        throw std::runtime_error("Failed to resolve types for " + fn_name);
    }

    llvm::FunctionType* function_type = llvm::FunctionType::get(
        return_type,
        param_types.value(),
        this->is_variadic()
    );

    llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        fn_name,
        module
    );
}

llvm::Value* AstFunctionDeclaration::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    llvm::Function* function = module->getFunction(this->get_internal_name());
    if (!function)
    {
        throw std::runtime_error("Function symbol missing: " + this->get_internal_name());
    }

    if (this->is_extern())
    {
        return function;
    }

    llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(module->getContext(), "entry", function);
    builder->SetInsertPoint(entry_bb);

    // We create a new builder for the prologue to ensure allocas are at the very top
    llvm::IRBuilder prologue_builder(&function->getEntryBlock(), function->getEntryBlock().begin());

    auto arg_it = function->arg_begin();
    for (const auto& param : this->get_parameters())
    {
        if (arg_it != function->arg_end())
        {
            arg_it->setName(param->get_name() + ".arg");

            // Create memory slot on the stack for the parameter
            llvm::AllocaInst* alloca = prologue_builder.CreateAlloca(
                arg_it->getType(), nullptr, param->get_name()
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
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->get_body());
            synthesisable != nullptr)
        {
            last_val = synthesisable->codegen(registry, module, builder);
        }
    }

    // Final Safety: Implicit Return
    // If the get_body didn't explicitly return (no terminator found), add one.
    if (llvm::BasicBlock* current_bb = builder->GetInsertBlock();
        current_bb && !current_bb->getTerminator())
    {
        if (llvm::Type* ret_type = function->getReturnType(); ret_type->isVoidTy())
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
                throw std::runtime_error("Function " + this->get_name() + " missing return path.");
            }
        }
    }

    if (llvm::verifyFunction(*function, &llvm::errs()))
    {
        function->print(llvm::errs(), nullptr); // Print IR to console to see what's wrong
        throw std::runtime_error("LLVM Function Verification Failed for: " + this->get_name());
    }

    return function;
}

bool stride::ast::is_fn_declaration(const TokenSet& tokens)
{
    return tokens.peek_next_eq(TokenType::KEYWORD_FN)
        || (tokens.peek_eq(TokenType::KEYWORD_EXTERN, 0)
            && tokens.peek_eq(TokenType::KEYWORD_FN, 1));
}


/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDeclaration> stride::ast::parse_fn_declaration(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& tokens
)
{
    int function_flags = 0;
    if (tokens.peek_next_eq(TokenType::KEYWORD_EXTERN))
    {
        tokens.next();
        function_flags |= SRFLAG_FN_DEF_EXTERN;
    }

    auto reference_token = tokens.expect(TokenType::KEYWORD_FN); // fn

    // Here we expect to receive the function name
    const auto fn_name_tok = tokens.expect(TokenType::IDENTIFIER, "Expected function name after 'fn'");
    const auto& fn_name = fn_name_tok.get_lexeme();

    auto function_scope = std::make_shared<SymbolRegistry>(registry, ScopeType::FUNCTION);

    tokens.expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    // If we don't receive a ')', the function has parameters, so we'll
    // have to parse it a little differenly
    if (!tokens.peek_next_eq(TokenType::RPAREN))
    {
        if (tokens.peek_next_eq(TokenType::THREE_DOTS))
        {
            parse_variadic_fn_param(function_scope, tokens, parameters);
        }
        else
        {
            parameters.push_back(parse_standalone_fn_param(function_scope, tokens));
        }

        parse_subsequent_fn_params(function_scope, tokens, parameters);
    }

    if (!parameters.empty() && parameters.back()->get_type()->is_variadic())
    {
        function_flags |= SRFLAG_FN_DEF_VARIADIC;
    }

    tokens.expect(TokenType::RPAREN, "Expected ')' after function parameters");
    tokens.expect(TokenType::COLON, "Expected a colon after function definition");

    // Return get_type doesn't have the same flags as the function, hence NONE
    auto return_type = parse_type(registry, tokens, "Expected return get_type in function header", SRFLAG_NONE);

    std::vector<std::unique_ptr<IAstType>> parameter_types_cloned;
    parameter_types_cloned.reserve(parameters.size());

    for (const auto& param : parameters)
    {
        parameter_types_cloned.push_back(param->get_type()->clone());
    }
    // Internal name contains all parameter types, so that there can be function overloads with
    // different parameter types
    std::string internal_name = fn_name;

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
        internal_name = resolve_internal_function_name(parameter_types, fn_name);
    }

    // Function will be defined in the parent registry (global, perhaps)
    // its children will reside in the function registry. This is intentional
    registry->define_function(fn_name, std::move(parameter_types_cloned), return_type->clone());

    std::unique_ptr<AstBlock> body = nullptr;

    if (function_flags & SRFLAG_FN_DEF_EXTERN)
    {
        tokens.expect(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
    }
    else
    {
        body = parse_block(function_scope, tokens);
    }

    return std::make_unique<AstFunctionDeclaration>(
        tokens.get_source(),
        reference_token.get_source_position(),
        registry,
        fn_name,
        internal_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        function_flags
    );
}

std::optional<std::vector<llvm::Type*>> AstFunctionDeclaration::resolve_parameter_types(llvm::Module* module) const
{
    std::vector<llvm::Type*> param_types;
    for (const auto& param : this->get_parameters())
    {
        auto llvm_type = internal_type_to_llvm_type(param->get_type(), module);
        if (!llvm_type)
        {
            return std::nullopt;
        }
        param_types.push_back(llvm_type);
    }
    return param_types;
}

std::string AstFunctionDeclaration::to_string()
{
    std::string params;
    for (const auto& param : this->get_parameters())
    {
        if (!params.empty())
            params += ", ";
        params += param->to_string();
    }

    const auto body_str = this->get_body() == nullptr ? "<empty>" : this->get_body()->to_string();

    return std::format(
        "FunctionDeclaration(name: {}(internal: {}), params: [{}], body: {}{} -> {})",
        this->get_name(),
        this->get_internal_name(),
        params,
        body_str,
        this->is_extern() ? " (extern)" : "",
        this->get_return_type()->to_string()
    );
}
