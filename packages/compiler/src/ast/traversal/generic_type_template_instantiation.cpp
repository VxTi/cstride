#include "ast/visitor.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

#include <format>
#include <ranges>

using namespace stride::ast;

void TemplateInstantiator::accept_function_call_node(SymbolTable* symbol_table, AstFunctionCall* function_call)
{
    if (function_call->get_generic_type_arguments().empty())
        return;


    add_generic_instantiation(
        function_call->get_function_name(),
        copy_generic_type_list(function_call->get_generic_type_arguments()),
        function_call
    );
    /*auto* definition = symbol_table->get_generic_function_definition(
        function_call->get_function_name(),
        function_call->get_arguments().size(),
        function_call->get_generic_type_arguments().size()
    ).value_or(nullptr);

    if (!definition)
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Could not find generic function definition for '{}'", function_call->get_function_name()),
            function_call->get_source_position()
        );
    }
    definition->add_generic_instantiation(
        symbol_table,
        copy_generic_type_list(function_call->get_generic_type_arguments())
    );*/
}

std::string TemplateInstantiator::format_generic_function_instantiation(
    std::string function_name,
    const std::vector<std::unique_ptr<IAstType>>& generic_parameter_types
)
{
    std::vector<std::string> type_names;
    type_names.reserve(generic_parameter_types.size());

    for (const auto& type : generic_parameter_types)
        type_names.push_back(type->get_type_name());

    return std::format("{}@{}", function_name, join(type_names, "_"));
}

void TemplateInstantiator::add_generic_instantiation(
    const std::string& function_name,
    const std::vector<std::unique_ptr<IAstType>>& generic_types,
    IAstNode* node
)
{
    const auto instantiation_key = format_generic_function_instantiation(function_name, generic_types);
    if (this->_instantiations.contains(instantiation_key))
        return;

    this->_instantiations[instantiation_key] = GenericFunctionTemplate{
        function_name,
        copy_generic_type_list(generic_types),
        node
    };
}

std::vector<GenericFunctionTemplate> TemplateInstantiator::get_generic_function_templates() const
{
    std::vector<GenericFunctionTemplate> templates;
    templates.reserve(this->_instantiations.size());

    for (const auto& [function_name, generic_types, node] : this->_instantiations | std::views::values)
    {
        templates.emplace_back(function_name, copy_generic_type_list(generic_types), node);
    }

    return templates;
}


std::vector<GenericTypeTemplate> TemplateInstantiator::get_generic_type_templates() const
{
    // TODO: Implement
    return {};
}
