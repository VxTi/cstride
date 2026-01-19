#include <iostream>
#include <sstream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "ast/nodes/expression.h"

using namespace stride::ast;

bool AstFunctionInvocation::is_reducible()
{
    // TODO: implement
    // Function calls can be reducible if the function returns
    // a constant value or if all arguments are reducible.
    return false;
}

IAstNode* AstFunctionInvocation::reduce()
{
    return this;
}

std::string AstFunctionInvocation::to_string()
{
    std::ostringstream oss;

    for (const auto& arg : this->get_arguments())
    {
        oss << arg->to_string() << ", ";
    }

    return std::format("FunctionInvocation({} {})", this->get_function_name(), oss.str());
}

llvm::Value* AstFunctionInvocation::codegen(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    const auto internal_name = this->get_function_name();
    // The actual function being called. This must be in the registry.
    llvm::Function* callee = module->getFunction(internal_name);
    if (!callee)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Function '" + internal_name + "' was not found in this scope"
            )
        );
    }

    if (callee->arg_size() != this->get_arguments().size())
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Incorrect arguments passed for function '" + internal_name + "'"
            )
        );
    }

    std::vector<llvm::Value*> args_v;
    for (const auto& arg : this->get_arguments())
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(arg.get()))
        {
            auto* arg_val = synthesisable->codegen(module, context, builder);
            if (!arg_val)
            {
                return nullptr;
            }
            args_v.push_back(arg_val);
        }
        else
        {
            std::cerr << "Argument is not synthesizable" << std::endl;
            return nullptr;
        }
    }

    return builder->CreateCall(callee, args_v, "calltmp");
}

std::string compose_function_name(TokenSet& tokens)
{
    const auto initial = tokens.expect(TokenType::IDENTIFIER, "Expected function name").lexeme;
    std::vector function_segments = {initial};

    while (tokens.peak_next_eq(TokenType::DOUBLE_COLON))
    {
        tokens.next();
        const auto next = tokens.expect(TokenType::IDENTIFIER, "Expected function name segment").lexeme;
        function_segments.push_back(next);
    }

    return internal_identifier_from_segments(function_segments);
}
