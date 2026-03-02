#include "ast/casting.h"
#include "ast/parsing_context.h"

using namespace stride::ast;
using namespace stride::ast::definition;

std::optional<FunctionDefinition*> ParsingContext::get_function_definition(
    const std::string& function_name,
    const std::vector<std::unique_ptr<IAstType>>& parameter_types
) const
{
    for (const auto& global_scope = this->traverse_to_root();
         const auto& symbol_def : global_scope._symbols)
    {
        if (auto* fn_def = dynamic_cast<FunctionDefinition*>(symbol_def.get()))
        {
            if (fn_def->matches_signature(function_name, parameter_types))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

std::optional<FunctionDefinition*> ParsingContext::get_function_definition(
    const std::string& function_name,
    IAstType* function_type
) const
{
    const auto signature = cast_type<AstFunctionType*>(function_type);

    if (!signature)
    {
        return std::nullopt;
    }

    for (const auto& global_scope = this->traverse_to_root();
         const auto& symbol_def : global_scope._symbols)
    {
        if (auto* fn_def = dynamic_cast<FunctionDefinition*>(symbol_def.get()))
        {
            if (fn_def->matches_signature(function_name, signature))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

bool FunctionDefinition::matches_signature(
    const std::string& name,
    const AstFunctionType* signature
) const
{
    if (!this->_function_type->get_return_type()->equals(*signature->get_return_type()))
        return false;

    const auto& other_params = signature->get_parameter_types();

    return matches_signature(name, other_params);

}

bool FunctionDefinition::matches_signature(
    const std::string& function_name,
    const std::vector<std::unique_ptr<IAstType>>& other_parameter_types
)
const
{
    if (this->get_function_name() != function_name)
        return false;

    const auto& self_params = this->_function_type->get_parameter_types();

    if ((this->get_flags() & SRFLAG_FN_DEF_VARIADIC) != 0)
    {
        if (other_parameter_types.size() < self_params.size())
            return false;

        // Otherwise, we expect the same amount of arguments
    }
    else
    {
        if (other_parameter_types.size() != self_params.size())
            return false;
    }

    for (size_t i = 0; i < self_params.size(); i++)
    {
        if (!self_params[i]->equals(*other_parameter_types[i]))
        {
            return false;
        }
    }

    return true;
}
