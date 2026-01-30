#include <complex>
#include <format>
#include <iostream>
#include <sstream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "formatting.h"
#include "ast/flags.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "../../../../include/ast/internal_names.h"

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

    std::vector<std::string> arg_types;
    for (const auto& arg : this->get_arguments())
    {
        arg_types.push_back(arg->to_string());
    }

    return std::format(
        "FunctionCall({} ({}) [{}])",
        this->get_function_name(),
        this->get_internal_name(),
        join(arg_types, ", ")
    );
}

std::string AstFunctionCall::format_suggestion(const ISymbolDef* suggestion)
{
    if (const auto fn_call = dynamic_cast<const SymbolFnDefinition*>(suggestion))
    {
        // We'll format the arguments
        std::vector<std::string> arg_types;

        for (const auto& arg : fn_call->get_parameter_types())
        {
            arg_types.push_back(arg->get_internal_name());
        }

        if (arg_types.empty()) arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));

        return std::format(
            "{}({})",
            fn_call->get_internal_symbol_name(),
            join(arg_types, ", ")
        );
    }

    return suggestion->get_internal_symbol_name();
}

std::string AstFunctionCall::format_function_name() const
{
    std::vector<std::string> arg_types;

    for (const auto& arg : this->_arguments)
    {
        const auto type = infer_expression_type(this->get_registry(), arg.get());

        arg_types.push_back(type->get_internal_name());
    }
    if (arg_types.empty()) arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));

    return std::format("{}({})", this->get_function_name(), join(arg_types, ", "));
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
        const auto suggested_alternative_symbol = scope->fuzzy_find(this->get_function_name());
        const auto suggested_alternative =
            suggested_alternative_symbol
                ? std::format("Did you mean '{}'?", this->format_suggestion(suggested_alternative_symbol))
                : "";

        throw parsing_error(
            ErrorType::RUNTIME_ERROR,
            std::format("Function '{}' was not found in this scope", this->format_function_name()),
            *this->get_source(),
            this->get_source_position(),
            suggested_alternative
        );
    }

    // Reduce last argument if variadic
    const auto minimum_arg_count = callee->arg_size() - (callee->isVarArg() ? 1 : 0);

    if (const auto provided_arg_count = this->get_arguments().size();
        provided_arg_count < minimum_arg_count)
    {
        throw parsing_error(
            ErrorType::RUNTIME_ERROR,
            std::format("Incorrect arguments passed for function '{}'", this->get_function_name()),
            *this->get_source(),
            this->get_source_position(),
            std::format("Incorrect arguments passed for function '{}'", this->get_function_name())
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
    const auto candidate_function_name = reference_token.get_lexeme();
    auto function_parameter_set = collect_parenthesized_block(set);

    std::vector<std::unique_ptr<AstExpression>> function_arg_nodes = {};
    std::vector<IAstInternalFieldType*> parameter_types = {};
    std::vector<std::unique_ptr<IAstInternalFieldType>> parameter_type_owners = {};

    // Parsing function parameter values
    if (function_parameter_set.has_value())
    {
        do
        {
            auto subset = function_parameter_set.value();
            auto initial_arg = parse_inline_expression(scope, subset);

            if (!initial_arg) break;

            auto initial_type = infer_expression_type(scope, initial_arg.get());

            parameter_types.push_back(initial_type.get());
            parameter_type_owners.push_back(std::move(initial_type));
            function_arg_nodes.push_back(std::move(initial_arg));

            // Consume next parameters
            while (subset.has_next())
            {
                subset.expect(TokenType::COMMA, "Expected ',' between function arguments");

                auto next_arg = parse_inline_expression(scope, subset);

                if (!next_arg)
                {
                    subset.throw_error("Expected expression for function argument");
                }

                auto next_type = infer_expression_type(scope, next_arg.get());
                parameter_types.push_back(next_type.get());
                parameter_type_owners.push_back(std::move(next_type));
                function_arg_nodes.push_back(std::move(next_arg));
            }
        }
        while (false);
    }

    std::string internal_fn_name = resolve_internal_function_name(
        parameter_types,
        candidate_function_name
    );

    return std::make_unique<AstFunctionCall>(
        set.get_source(),
        reference_token.get_source_position(),
        scope,
        candidate_function_name,
        internal_fn_name,
        std::move(function_arg_nodes)
    );
}
