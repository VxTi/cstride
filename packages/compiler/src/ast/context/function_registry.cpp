#include "ast/casting.h"
#include "ast/parsing_context.h"

using namespace stride::ast;
using namespace stride::ast::definition;

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
            if (fn_def->equals(function_name, signature))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

bool FunctionDefinition::equals(const std::string& name, const AstFunctionType* signature) const
{
    if (this->get_symbol().name != name)
        return false;

    const auto& self_params = this->_function_type->get_parameter_types();
    const auto& other_params = signature->get_parameter_types();

    if (self_params.size() != other_params.size())
    {
        return false;
    }

    if (!this->_function_type->get_return_type()->equals(*signature->get_return_type()))
    {
        return false;
    }

    for (size_t i = 0; i < self_params.size(); i++)
    {
        if (!self_params[i]->equals(*other_params[i]))
        {
            return false;
        }
    }

    return true;
}
