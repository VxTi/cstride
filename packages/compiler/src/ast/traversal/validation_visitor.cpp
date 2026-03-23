#include "ast/visitor.h"

using namespace stride::ast;

void ValidationVisitor::accept(SymbolTable* symbol_table, IAstNode* node)
{
    node->validate(symbol_table);
}
