#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void AstFunctionCall::resolve_forward_references(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    // Add generic types to function definition's generic instantiations
    if (!this->_generic_type_arguments.empty())
    {
        const auto& definition = this->get_function_definition();

        definition->add_generic_instantiation(
            copy_generic_type_list(this->_generic_type_arguments)
        );
    }

    for (const auto& arg : this->_arguments)
        arg->resolve_forward_references(module, builder);
}
