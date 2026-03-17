#include "ast/visitor.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void FunctionCallVisitor::accept(AstFunctionCall* function_call)
{
    if (function_call->get_generic_type_arguments().empty())
        return;

    auto* definition = function_call->get_function_definition();
    if (auto* fn_def = dynamic_cast<definition::FunctionDefinition*>(definition))
    {
        fn_def->add_generic_instantiation(
            copy_generic_type_list(function_call->get_generic_type_arguments())
        );
    }
}
