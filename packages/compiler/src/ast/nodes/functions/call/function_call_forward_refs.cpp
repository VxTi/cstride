#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void AstFunctionCall::resolve_forward_references(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    for (const auto& arg : this->_arguments)
        arg->resolve_forward_references(module, builder);
}
