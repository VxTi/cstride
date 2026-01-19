#include "ast/nodes/function_definition.h"

#include "ast/nodes/blocks.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>


using namespace stride::ast;

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
        auto initial = parse_first_fn_param(scope, tokens);
        parameters.push_back(std::move(initial));

        parse_subsequent_fn_params(scope, tokens, parameters);
    }

    // Internal name contains all parameter types, so that there can be function overloads with
    // different parameter types
    std::string internal_name = fn_name;

    // Prevent tagging extern functions with different internal names.
    // This prevents the linker from being unable to make a reference to this function.
    if ((flags & SRFLAG_FN_DEF_EXTERN) == 0)
    {
        for (const auto& param : parameters)
        {
            internal_name += SEGMENT_DELIMITER + param->get_type()->get_internal_name();
        }
    }
    scope.try_define_scoped_symbol(*tokens.source(), fn_name_tok, fn_name, SymbolType::FUNCTION, internal_name);

    // The return type of the function, e.g., ": i8"
    tokens.expect(TokenType::RPAREN);
    tokens.expect(TokenType::COLON);
    std::unique_ptr<types::AstType> return_type = types::parse_primitive_type(tokens);

    std::unique_ptr<AstBlock> body = nullptr;

    if (flags & SRFLAG_FN_DEF_EXTERN)
    {
        tokens.expect(TokenType::SEMICOLON);
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

    return std::move(std::make_unique<AstFunctionDeclaration>(
        tokens.source(),
        reference_token.offset,
        fn_name,
        internal_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        flags
    ));
}


std::optional<std::vector<llvm::Type*>> resolve_parameter_types(
    const AstFunctionDeclaration* self,
    llvm::Module* module,
    llvm::LLVMContext& context
)
{
    std::vector<llvm::Type*> param_types;
    for (const auto& param : self->get_parameters())
    {
        auto llvm_type = types::internal_type_to_llvm_type(param->get_type(), module, context);
        if (!llvm_type)
        {
            return std::nullopt;
        }
        param_types.push_back(llvm_type);
    }
    return param_types;
}

llvm::Value* AstFunctionDeclaration::codegen(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* irBuilder
)
{
    const auto fn_name = this->get_internal_name();

    // Create parameter types vector
    const std::optional<std::vector<llvm::Type*>> param_types = resolve_parameter_types(this, module, context);
    if (!param_types)
    {
        std::cerr << "Failed to resolve parameter types for function " << fn_name << std::endl;
        return nullptr;
    }

    // Create function type
    llvm::Type* return_type = types::internal_type_to_llvm_type(this->return_type().get(), module, context);
    if (!return_type)
    {
        // This can currently happen when unidentified symbols are used as types
        std::cerr << "Failed to resolve return type for function " << fn_name << std::endl;
        return nullptr;
    }
    llvm::FunctionType* function_type = llvm::FunctionType::get(
        return_type,
        param_types.value(),
        this->is_variadic()
    );

    // Create the function
    llvm::Function* function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        fn_name,
        module
    );

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

    // Generate body code
    llvm::Value* ret_val = nullptr;

    if (this->body() != nullptr)
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(this->body()))
        {
            ret_val = synthesisable->codegen(module, context, &builder);
        }
    }

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
