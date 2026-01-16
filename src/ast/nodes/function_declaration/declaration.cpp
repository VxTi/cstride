#include "ast/nodes/declaration.h"

#include "ast/nodes/blocks.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>


using namespace stride::ast;

std::string AstFunctionDefinitionNode::to_string()
{
    std::string params;
    for (const auto& param : this->parameters())
    {
        if (!params.empty())
            params += ", ";
        params += param->to_string();
    }

    const auto body_str = this->body() == nullptr ? "EMPTY" : this->body()->to_string();

    return std::format(
        "FunctionDefinition(name: {}, parameters: [ {} ], body: {}, is_external: {})",
        this->name().value,
        params,
        body_str,
        this->is_extern()
    );
}

bool AstFunctionDefinitionNode::can_parse(const TokenSet& tokens)
{
    return tokens.peak_next_eq(TokenType::KEYWORD_FN) || tokens.peak_next_eq(TokenType::KEYWORD_EXTERNAL);
}


/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDefinitionNode> AstFunctionDefinitionNode::try_parse(
    const Scope& scope,
    TokenSet& tokens
)
{
    bool is_external = false;
    if (tokens.peak_next_eq(TokenType::KEYWORD_EXTERNAL))
    {
        tokens.expect(TokenType::KEYWORD_EXTERNAL);
        is_external = true;
    }

    tokens.expect(TokenType::KEYWORD_FN);

    const auto fn_name_tok = tokens.expect(TokenType::IDENTIFIER);
    const auto fn_name = Symbol(fn_name_tok.lexeme);
    scope.try_define_scoped_symbol(*tokens.source(), fn_name_tok, fn_name);

    tokens.expect(TokenType::LPAREN);
    std::vector<std::unique_ptr<AstFunctionParameterNode>> parameters = {};

    if (!tokens.peak_next_eq(TokenType::RPAREN))
    {
        auto initial = AstFunctionParameterNode::try_parse(scope, tokens);
        parameters.push_back(std::move(initial));

        AstFunctionParameterNode::try_parse_subsequent_parameters(scope, tokens, parameters);
    }

    tokens.expect(TokenType::RPAREN);
    tokens.expect(TokenType::COLON);

    const Scope function_scope(scope, ScopeType::FUNCTION);

    std::unique_ptr<types::AstType> return_type = types::try_parse_type(tokens);
    std::unique_ptr<AstBlockNode> body = nullptr;

    if (is_external)
    {
        tokens.expect(TokenType::SEMICOLON);
    }
    else
    {
        body = AstBlockNode::try_parse_block(function_scope, tokens);

        if (body != nullptr && body->children().empty())
        {
            // Ensure we don't populate the function if it doesn't actually have resolved AST nodes
            body = nullptr;
        }
    }

    return std::move(std::make_unique<AstFunctionDefinitionNode>(
        fn_name,
        std::move(parameters),
        std::move(return_type),
        std::move(body),
        is_external
    ));
}


llvm::Value* AstFunctionDefinitionNode::codegen(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* irBuilder
    )
{
    // Create parameter types vector
    std::vector<llvm::Type*> param_types;
    for (const auto& param : this->parameters())
    {
        auto llvm_type = types::ast_type_to_llvm(param->type.get(), context);
        if (!llvm_type) {
             std::cerr << "Failed to resolve type for parameter " << param->name.value << std::endl;
             return nullptr;
        }
        param_types.push_back(llvm_type);
    }

    // Create function type
    llvm::Type* return_type = types::ast_type_to_llvm(this->return_type().get(), context);
    if (!return_type) {
         std::cerr << "Failed to resolve return type for function " << this->name().value << std::endl;
         return nullptr;
    }
    llvm::FunctionType* function_type = llvm::FunctionType::get(
        return_type,
        param_types,
        false // is not variadic
    );

    // Create the function
    llvm::Function* function = llvm::Function::Create(
        function_type,
        llvm::Function::ExternalLinkage,
        this->name().value,
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
    if (!builder.GetInsertBlock()->getTerminator())
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

    if (llvm::verifyFunction(*function, &llvm::errs())) {
        std::cerr << "Function " << this->name().value << " verification failed!" << std::endl;
    }

    return function;
}
