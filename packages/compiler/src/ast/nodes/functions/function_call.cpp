#include <format>
#include <sstream>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "formatting.h"
#include "ast/symbols.h"
#include "ast/optionals.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"

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
    arg_types.reserve(this->_arguments.size());

    for (const auto& arg : this->_arguments)
    {
        const auto type = infer_expression_type(this->get_registry(), arg.get());

        arg_types.push_back(type->get_internal_name());
    }
    if (arg_types.empty()) arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));

    return std::format("{}({})", this->get_function_name(), join(arg_types, ", "));
}

llvm::Value* AstFunctionCall::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
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
        const auto suggested_alternative_symbol = registry->fuzzy_find(this->get_function_name());
        const auto suggested_alternative =
            suggested_alternative_symbol
                ? std::format("Did you mean '{}'?", format_suggestion(suggested_alternative_symbol))
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
    const auto& arguments = this->get_arguments();

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        const auto arg_val = arguments[i]->codegen(registry, module, builder);

        if (!arg_val)
        {
            return nullptr;
        }

        llvm::Value* final_val = arg_val;

        // Determine if we need to unwrap based on the target function signature
        if (i < callee->arg_size())
        {
            // Check for strict type equality.
            // If the argument is Optional<T> but the function expects T, we unwrap.
            if (const llvm::Type* expected_type = callee->getFunctionType()->getParamType(i);
                arg_val->getType() != expected_type)
            {
                final_val = unwrap_optional_value(arg_val, builder);
            }
        }
        else
        {
            // For variadic arguments, we default to eager unwrapping
            final_val = unwrap_optional_value(arg_val, builder);
        }

        args_v.push_back(final_val);
    }

    return builder->CreateCall(callee, args_v, "calltmp");
}

std::unique_ptr<AstExpression> stride::ast::parse_function_call(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    const auto reference_token = set.next();
    const auto& candidate_function_name = reference_token.get_lexeme();
    auto function_parameter_set = collect_parenthesized_block(set);

    std::vector<std::unique_ptr<AstExpression>> function_arg_nodes = {};
    std::vector<IAstType*> parameter_types = {};
    std::vector<std::unique_ptr<IAstType>> parameter_type_owners = {};

    // Parsing function parameter values
    if (function_parameter_set.has_value())
    {
        auto subset = function_parameter_set.value();
        auto initial_arg = parse_inline_expression(registry, subset);

        if (initial_arg)
        {
            // TODO: Evaluate this. One might not be able to infer expression types if they invoke functions that
            //  haven't been declared yet, hence yielding in a
            auto initial_type = infer_expression_type(registry, initial_arg.get());

            parameter_types.push_back(initial_type.get());
            parameter_type_owners.push_back(std::move(initial_type));
            function_arg_nodes.push_back(std::move(initial_arg));

            // Consume next parameters
            while (subset.has_next())
            {
                const auto preceding = subset.expect(TokenType::COMMA, "Expected ',' between function arguments");

                auto next_arg = parse_inline_expression(registry, subset);

                if (!next_arg)
                {
                    // Since the RParen is already consumed, we have to manually extract its position with the following assumption
                    // It's possible this yields END_OF_FILE
                    const auto len = set.at(set.position() - 1).get_source_position().offset - 1 - preceding.
                        get_source_position().offset;
                    throw parsing_error(
                        ErrorType::SYNTAX_ERROR,
                        "Expected expression for function argument",
                        *subset.get_source(),
                        SourcePosition(
                            preceding.get_source_position().offset + 1,
                            len
                        )
                    );
                }

                auto next_type = infer_expression_type(registry, next_arg.get());
                parameter_types.push_back(next_type.get());
                parameter_type_owners.push_back(std::move(next_type));
                function_arg_nodes.push_back(std::move(next_arg));
            }
        }
    }

    Symbol internal_fn_sym = resolve_internal_function_name(
        parameter_types,
        candidate_function_name
    );

    return std::make_unique<AstFunctionCall>(
        set.get_source(),
        // TODO: Fix this. This currently refers only to the first token, instead of the full call
        reference_token.get_source_position(),
        registry,
        internal_fn_sym,
        std::move(function_arg_nodes)
    );
}
