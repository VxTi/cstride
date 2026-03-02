#pragma once

#include "traversal.h"

namespace stride::ast
{
    class IAstExpression;

    /// Visitor that infers and assigns types to every expression node in the AST.
    ///
    /// Used together with AstNodeTraverser: the traverser drives bottom-up traversal,
    /// and this visitor is called for each expression after all of its child expressions
    /// have already been typed.  In addition to setting types, the visitor registers
    /// variable declarations and function declarations in the appropriate parsing context,
    /// matching the work previously performed by the scattered resolve_types() calls.
    class TypeInferenceVisitor : public IVisitor
    {
    public:
        /// Infers the type of `expr` and stores it on the node.
        /// For AstVariableDeclaration: also registers the variable in its context.
        /// For IAstFunction: also registers the function in its context.
        void accept(IAstExpression* expr) override;
    };
}
