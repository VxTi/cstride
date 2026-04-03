#include "ast/visitor.h"

using namespace stride::ast;

void ForwardReferenceInitializer::accept(SymbolTable* symbol_table, IAstNode* node)
{
    node->resolve_forward_references(symbol_table, this->_module, this->_ir_builder);
}
