#include <iostream>
#include <sstream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

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

std::optional<std::string> resolve_type_name(AstExpression* expr)
{
    if (!expr) return std::nullopt;

    if (const auto* primitive = dynamic_cast<AstPrimitiveType*>(expr))
    {
        return primitive_type_to_str(primitive->type());
    }

    if (const auto* custom_type = dynamic_cast<AstNamedType*>(expr))
    {
        return custom_type->name();
    }

    return std::nullopt;
}

llvm::Value* AstFunctionInvocation::codegen(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    std::string candidate_name = this->get_function_name();

    // 2. Append argument types to match the definition's mangling logic
    // matching: internal_name += SEGMENT_DELIMITER + param->get_type()->get_internal_name();
    for (const auto& arg_expr : this->get_arguments()) {
        // Note: You must evaluate/resolve the type of the argument expression first
        if (const auto arg_type = resolve_type_name(&*arg_expr); arg_type.has_value())
        {
            candidate_name += SEGMENT_DELIMITER + arg_type.value();
        }
    }

    // 3. Attempt to find the mangled name in the LLVM module
    llvm::Function* callee = module->getFunction(candidate_name);

    // 4. Fallback for 'extern' functions
    // In parse_fn_declaration, externs do not have type suffixes.
    // If the mangled version isn't found, try the raw name.
    if (!callee) {
        callee = module->getFunction(this->get_function_name());
    }

    if (!callee)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Function '" + candidate_name + "' was not found in this scope"
            )
        );
    }

    if (callee->arg_size() != this->get_arguments().size())
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                "Incorrect arguments passed for function '" + candidate_name + "'"
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
