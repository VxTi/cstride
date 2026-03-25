#include "ast/visitor.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void FunctionCallVisitor::accept_function_call(AstFunctionCall* function_call)
{
    if (function_call->get_generic_type_arguments().empty())
        return;

    auto* definition = function_call->get_context()->get_generic_function_definition(
        function_call->get_function_name(),
        function_call->get_generic_type_arguments().size()
    ).value_or(nullptr);

    if (!definition)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Could not find generic function definition for '{}'", function_call->get_function_name()),
            function_call->get_source_fragment()
        );
    }
    definition->add_generic_instantiation(
        copy_generic_type_list(function_call->get_generic_type_arguments())
    );
}
