#include <sstream>

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

llvm::Value* AstFunctionInvocation::codegen(llvm::Module* module, llvm::LLVMContext& context)
{
    return nullptr;
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