#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/definitions/function_definition.h"

#include <algorithm>

using namespace stride::ast;
using namespace stride::ast::definition;

std::optional<FunctionDefinition*> ParsingContext::get_function_definition(
    const std::string& function_name,
    const std::vector<std::unique_ptr<IAstType>>& parameter_types,
    const size_t instantiated_generic_count
) const
{
    for (const auto& global_scope = this->traverse_to_root();
         const auto& symbol_def : global_scope._symbols)
    {
        if (auto* fn_def = dynamic_cast<FunctionDefinition*>(symbol_def.get()))
        {
            if (fn_def->matches_parameter_signature(
                function_name,
                parameter_types,
                instantiated_generic_count))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

std::optional<FunctionDefinition*> ParsingContext::get_function_definition(
    const std::string& function_name,
    // We might call this function with an anonymous type, hence not having `AstFunctionType`
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
            if (fn_def->matches_type_signature(function_name, signature))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

bool FunctionDefinition::matches_type_signature(
    const std::string& name,
    const AstFunctionType* signature
) const
{
    if (!this->_function_type->get_return_type()->equals(signature->get_return_type().get()))
        return false;

    const auto& other_params = signature->get_parameter_types();

    return matches_parameter_signature(
        name,
        other_params,
        signature->get_generic_parameter_names().size()
    );
}

bool FunctionDefinition::matches_parameter_signature(
    const std::string& internal_function_name,
    const std::vector<std::unique_ptr<IAstType>>& other_parameter_types,
    const size_t generic_argument_count
)
const
{
    if (this->get_internal_symbol_name() != internal_function_name)
        return false;

    // Ensure we have the right generic overload variant of this function.
    // This allows us to create several functions with the same signature / name, but with
    // different generic parameter overloads.
    //
    // For generic overloads, we just check whether the name and generic count is equal.
    if (!this->_function_type->get_generic_parameter_names().empty() && generic_argument_count > 0)
        return true;

    const auto& self_params = this->_function_type->get_parameter_types();

    if (this->is_variadic())
    {
        if (other_parameter_types.size() < self_params.size())
            return false;
    }
    else
    {
        // Otherwise, we expect the same number of arguments
        if (other_parameter_types.size() != self_params.size())
            return false;
    }

    for (size_t i = 0; i < self_params.size(); i++)
    {
        // Strict equality check - parameters must match exactly,
        // otherwise named overloading with different signatures wouldn't work.
        if (!self_params[i]->equals(other_parameter_types[i].get()))
        {
            return false;
        }
    }

    return true;
}

void ParsingContext::define_function(
    Symbol function_name,
    std::unique_ptr<AstFunctionType> function_type,
    const VisibilityModifier visibility,
    const int flags
) const
{
    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());

    if (this->is_function_defined_globally(function_name.internal_name, function_type.get()))
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Function '{}' already defined globally", function_name.name),
            function_name.symbol_position
        );
    }

    global_scope._symbols.push_back(
        std::make_unique<FunctionDefinition>(std::move(function_type), function_name, visibility, flags)
    );
}

bool ParsingContext::is_function_defined_globally(
    const std::string& function_name,
    const AstFunctionType* function_type
) const
{
    return std::ranges::any_of(
        this->traverse_to_root()._symbols,
        [&](const auto& symbol)
        {
            if (const auto* fn_def = dynamic_cast<const FunctionDefinition*>(symbol.get()))
            {
                return fn_def->matches_type_signature(function_name, function_type);
            }
            return false;
        }
    );
}


bool FunctionDefinition::has_generic_instantiation(const std::vector<std::unique_ptr<IAstType>>& generic_types) const
{
    for (const auto& [instantiated_generic_types, llvm_function, node] : this->_generic_overloads)
    {
        bool all_equal = true;
        for (size_t i = 0; i < generic_types.size(); i++)
        {
            if (!instantiated_generic_types[i]->equals(generic_types[i].get()))
            {
                all_equal = false;
                break;
            }
        }
        if (all_equal)
        {
            return true;
        }
    }

    return false;
}

void FunctionDefinition::add_generic_instantiation(GenericTypeList generic_overload_types)
{
    if (has_generic_instantiation(generic_overload_types))
        return; // Already instantiated

    // All other fields will be populated in later stages
    this->_generic_overloads.push_back({
        std::move(generic_overload_types)
    });
}
