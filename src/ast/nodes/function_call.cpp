#include <iostream>
#include <sstream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "ast/nodes/expression.h"

using namespace stride::ast;


std::string AstFunctionInvocation::to_string()
{
    std::ostringstream oss;

    for (const auto& arg : arguments)
    {
        oss << arg->to_string() << ", ";
    }

    return std::format("FunctionInvocation({} {})", function_name.to_string(), oss.str());
}

llvm::Value* AstFunctionInvocation::codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder)
{
    llvm::Function* callee = module->getFunction(function_name.value);
    if (!callee)
    {
        std::cerr << "Unknown function referenced: " << function_name.value << std::endl;
        return nullptr;
    }

    if (callee->arg_size() != arguments.size()) {
        std::cerr << "Incorrect arguments passed for function " << function_name.value << std::endl;
        return nullptr;
    }

    std::vector<llvm::Value*> args_v;
    for (const auto& arg : arguments)
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

Symbol consume_function_name(TokenSet& tokens)
{
    const auto initial = tokens.expect(TokenType::IDENTIFIER, "Expected function name").lexeme;
    std::vector function_segments = {initial};

    while (tokens.peak_next_eq(TokenType::DOUBLE_COLON))
    {
        tokens.next();
        const auto next = tokens.expect(TokenType::IDENTIFIER, "Expected function name segment").lexeme;
        function_segments.push_back(next);
    }

    return Symbol::from_segments(function_segments);
}