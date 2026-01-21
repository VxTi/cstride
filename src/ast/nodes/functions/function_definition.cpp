#include "ast/nodes/function_definition.h"

#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include "ast/nodes/blocks.h"
#include "ast/nodes/functions.h"


using namespace stride::ast;

void AstFunctionDeclaration::define_symbols(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    const auto fn_name = this->get_internal_name();

    // Create parameter types vector
    const std::optional<std::vector<llvm::Type*>> param_types = resolve_parameter_types(module, context);
    if (!param_types)
    {
        std::cerr << "Failed to resolve parameter types for function " << fn_name << std::endl;
        return;
    }

    // Create function type
    llvm::Type* return_type = internal_type_to_llvm_type(this->return_type().get(), module, context);
    if (!return_type)
    {
        // This can currently happen when unidentified symbols are used as types
        std::cerr << "Failed to resolve return type for function " << fn_name << std::endl;
        return;
    }
    llvm::FunctionType* function_type = llvm::FunctionType::get(
        return_type,
        param_types.value(),
        this->is_variadic()
    );

    // Create the function
    const llvm::Function* function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        fn_name,
        module
    );

    if (llvm::verifyFunction(*function, &llvm::errs()))
    {
        throw std::runtime_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Failed to verify function " + this->get_name()
            )
        );
    }
}

llvm::Value* AstFunctionDeclaration::codegen(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* irBuilder
)
{
    llvm::Function* function = module->getFunction(this->get_internal_name());
    if (!function)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Function '" + this->get_internal_name() + "' was not found in this scope"
            )
        );
    }

    if (this->is_extern())
    {
        return function;
    }

    // Create entry basic block (already appended to function by passing function as 3rd arg)
    llvm::BasicBlock* entry_block = llvm::BasicBlock::Create(
        context,
        "entry",
        function
    );

    // Set up IR builder
    llvm::IRBuilder builder(context);
    builder.SetInsertPoint(entry_block);

    // Register function arguments in the symbol table
    auto arg_it = function->arg_begin();
    for (const auto& param : this->get_parameters())
    {
        if (arg_it == function->arg_end())
        {
            break;
        }
        arg_it->setName(param->get_name());
        ++arg_it;
    }

    // Generate body code
    llvm::Value* ret_val = nullptr;

    if (this->body() != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->body()))
        {
            ret_val = synthesisable->codegen(module, context, &builder);

            // Void instructions cannot have a name, but the generator might have assigned one
            // This way, we prevent LLVM from complaining
            if (ret_val && ret_val->getType()->isVoidTy() && ret_val->hasName())
            {
                ret_val->setName("");
            }
        }
    }

    const auto return_type = llvm::cast<llvm::FunctionType>(function->getFunctionType())->getReturnType();

    // Add default return if needed (void functions or missing return)
    if (auto* block = builder.GetInsertBlock(); block && !block->getTerminator())
    {
        if (return_type->isVoidTy())
        {
            builder.CreateRetVoid();
        }
        else if (ret_val)
        {
            builder.CreateRet(ret_val);
        }
    }

    if (llvm::verifyFunction(*function, &llvm::errs()))
    {
        std::cerr << "Function " << this->get_name() << " verification failed!" << std::endl;
        std::cerr << &llvm::errs() << std::endl;
        return nullptr;
    }

    return function;
}

bool stride::ast::is_fn_declaration(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_FN)
        || (tokens.peak_eq(TokenType::KEYWORD_EXTERN, 0)
            && tokens.peak_eq(TokenType::KEYWORD_FN, 1));
}


/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDeclaration> stride::ast::parse_fn_declaration(
    const Scope& scope,
    TokenSet& tokens
)
{
    // TODO: Add support for variadic arguments
    int flags = 0;
    if (tokens.peak_next_eq(TokenType::KEYWORD_EXTERN))
    {
        tokens.expect(TokenType::KEYWORD_EXTERN);
        flags |= SRFLAG_FN_DEF_EXTERN;
    }

    auto reference_token = tokens.expect(TokenType::KEYWORD_FN); // fn

    // Here we expect to receive the function name
    const auto fn_name_tok = tokens.expect(TokenType::IDENTIFIER);
    const auto fn_name = fn_name_tok.lexeme;

    tokens.expect(TokenType::LPAREN);
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    // If we don't receive a ')', the function has parameters, so we'll
    // have to parse it a little differenly
    if (!tokens.peak_next_eq(TokenType::RPAREN))
    {
        if (parameters.size() > MAX_FUNCTION_PARAMETERS)
        {
            throw parsing_error(make_ast_error(
                *tokens.source(),
                reference_token.offset,
                "Function cannot have more than " + std::to_string(MAX_FUNCTION_PARAMETERS) + " parameters"
            ));
        }

        auto initial = parse_standalone_fn_param(scope, tokens);
        parameters.push_back(std::move(initial));

        parse_subsequent_fn_params(scope, tokens, parameters);
    }

    tokens.expect(TokenType::RPAREN, "Expected ')' after function parameters");
    tokens.expect(TokenType::COLON, "Expected a colon after function header type");
    std::unique_ptr<IAstInternalFieldType> return_type = parse_ast_type(tokens);


    std::vector<std::unique_ptr<IAstInternalFieldType>> parameter_types = {};
    for (const auto& param : parameters)
    {
        parameter_types.push_back(param->get_type()->clone());
    }
    // Internal name contains all parameter types, so that there can be function overloads with
    // different parameter types
    std::string internal_name = fn_name;

    // Prevent tagging extern functions with different internal names.
    // This prevents the linker from being unable to make a reference to this function.
    if ((flags & SRFLAG_FN_DEF_EXTERN) == 0)
    {
        internal_name = resolve_internal_function_name(parameter_types, fn_name);
    }
    scope.try_define_scoped_symbol(*tokens.source(), fn_name_tok, fn_name, SymbolType::FUNCTION, internal_name);

    std::unique_ptr<AstBlock> body = nullptr;

    if (flags & SRFLAG_FN_DEF_EXTERN)
    {
        tokens.expect(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
    }
    else
    {
        const Scope function_scope(scope, ScopeType::FUNCTION);
        body = parse_block(function_scope, tokens);

        if (body != nullptr && body->children().empty())
        {
            // Ensure we don't populate the function if it doesn't actually have resolved AST nodes
            body = nullptr;
        }
    }

    return std::make_unique<AstFunctionDeclaration>(
        tokens.source(),
        reference_token.offset,
        fn_name,
        internal_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        flags
    );
}

std::optional<std::vector<llvm::Type*>> AstFunctionDeclaration::resolve_parameter_types(
    llvm::Module* module,
    llvm::LLVMContext& context
) const
{
    std::vector<llvm::Type*> param_types;
    for (const auto& param : this->get_parameters())
    {
        auto llvm_type = internal_type_to_llvm_type(param->get_type(), module, context);
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

    const auto body_str = this->body() == nullptr ? "<empty>" : this->body()->to_string();

    return std::format(
        "FunctionDefinition(name: {} ({}), parameters: [ {} ], body: {}{})",
        this->get_name(),
        this->get_internal_name(),
        params,
        body_str,
        this->is_extern() ? " (extern)" : ""
    );
}
