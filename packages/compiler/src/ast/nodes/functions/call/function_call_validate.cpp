#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void AstFunctionCall::validate()
{
    for (const auto& arg : this->_arguments)
    {
        arg->validate();
    }
}
