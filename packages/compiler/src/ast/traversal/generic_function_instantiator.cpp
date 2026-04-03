#include "ast/visitor.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void GenericFunctionInstantiator::accept_function_call_node(SymbolTable* symbol_table, AstFunctionCall* function_call)
{
    if (function_call->get_generic_type_arguments().empty())
        return;

    auto* definition = symbol_table->get_generic_function_definition(
        function_call->get_function_name(),
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
    );
}