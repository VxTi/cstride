#include <format>
#include <iostream>
#include <sstream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "ast/flags.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/functions.h"

using namespace stride::ast;


bool AstFunctionCall::is_reducible()
{
    // TODO: implement
    // Function calls can be reducible if the function returns
    // a constant value or if all arguments are reducible.
    return false;
}

IAstNode* AstFunctionCall::reduce()
{
    return this;
}

std::string AstFunctionCall::to_string()
{
    std::ostringstream oss;

    const auto& args = this->get_arguments();
    for (size_t i = 0; i < args.size(); ++i)
    {
        oss << args[i]->to_string();
        if (i < args.size() - 1)
        {
            oss << ", ";
        }
    }

    return std::format(
        "FunctionCall({} ({}) [{}])",
        this->get_function_name(),
        this->get_internal_name(),
        oss.str()
    );
}

std::optional<std::string> resolve_type_name(AstExpression* expr)
{
    if (!expr) return std::nullopt;

    if (const auto* primitive = dynamic_cast<AstPrimitiveFieldType*>(expr))
    {
        return primitive_type_to_str(primitive->type());
    }

    if (const auto* custom_type = dynamic_cast<AstNamedValueType*>(expr))
    {
        return custom_type->name();
    }

    return std::nullopt;
}

llvm::Value* AstFunctionCall::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    llvm::Function* callee = module->getFunction(this->get_internal_name());

    // For non-extern functions,
    if (!callee)
    {
        // It's possible that the function is internally registered with its normal name
        // This always happens for extern functions.
        callee = module->getFunction(this->get_function_name());
    }

    if (!callee)
    {
        auto suggested_alternative = scope->fuzzy_find(this->get_function_name());

        throw parsing_error(
            make_ast_error(
                ErrorType::RUNTIME_ERROR,
                *this->source,
                this->source_offset,
                std::format("Function '{}' was not found in this scope", this->get_function_name()),
                suggested_alternative
                    ? std::format(
                        "Did you mean '{}'?",
                        suggested_alternative->get_internal_symbol_name()
                    )
                    : ""
            )
        );
    }

    // Reduce last argument if variadic
    const auto minimum_arg_count = callee->arg_size() - (callee->isVarArg() ? 1 : 0);
    const auto provided_arg_count = this->get_arguments().size();

    if (provided_arg_count < minimum_arg_count)
    {
        throw parsing_error(
            make_ast_error(
                *this->source,
                this->source_offset,
                std::format("Incorrect arguments passed for function '{}'", this->get_function_name())
            )
        );
    }

    std::vector<llvm::Value*> args_v;
    for (const auto& arg : this->get_arguments())
    {
        if (auto* synthesisable = dynamic_cast<ISynthesisable*>(arg.get()))
        {
            auto arg_val = synthesisable->codegen(scope, module, context, builder);

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

std::unique_ptr<AstExpression> stride::ast::parse_function_call(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.next();
    const auto candidate_function_name = reference_token.lexeme;
    auto function_parameter_set = collect_parenthesized_block(set);

    std::vector<std::unique_ptr<AstExpression>> function_arg_nodes = {};
    std::vector<IAstInternalFieldType*> parameter_types = {};
    std::vector<std::unique_ptr<IAstInternalFieldType>> parameter_type_owners = {};

    // Parsing function parameter values
    if (function_parameter_set.has_value())
    {
        auto subset = function_parameter_set.value();
        auto initial_arg = parse_expression_extended(0, scope, subset);

        auto initial_type = infer_expression_type(scope, initial_arg.get());
        parameter_types.push_back(initial_type.get());
        parameter_type_owners.push_back(std::move(initial_type));
        function_arg_nodes.push_back(std::move(initial_arg));

        // Consume next parameters
        while (subset.has_next())
        {
            subset.expect(TokenType::COMMA);
            auto next_arg = parse_expression_extended(0, scope, subset);

            auto next_type = infer_expression_type(scope, next_arg.get());
            parameter_types.push_back(next_type.get());
            parameter_type_owners.push_back(std::move(next_type));
            function_arg_nodes.push_back(std::move(next_arg));
        }
    }

    std::string internal_fn_name = resolve_internal_function_name(
        parameter_types,
        candidate_function_name
    );

    return std::make_unique<AstFunctionCall>(
        set.source(),
        reference_token.offset,
        scope,
        candidate_function_name,
        internal_fn_name,
        std::move(function_arg_nodes)
    );
}
