#pragma once

#include "nodes/traversal.h"

namespace stride::ast
{
    class AstImport;
    class AstPackage;
    class AstFunctionDeclaration;
    class IAstExpression;

    /// Visitor that infers and assigns types to every expression node in the AST.
    ///
    /// Used together with AstNodeTraverser: the traverser drives bottom-up traversal,
    /// and this visitor is called for each expression after all of its child expressions
    /// have already been typed.  In addition to setting types, the visitor registers
    /// variable declarations and function declarations in the appropriate parsing context,
    /// matching the work previously performed by the scattered resolve_types() calls.
    class ExpressionVisitor : public IVisitor
    {
    public:
        /// Infers the type of `expr` and stores it on the node.
        /// For AstVariableDeclaration: also registers the variable in its context.
        /// For IAstFunction: also registers the function in its context.
        void accept(IAstExpression* expr) override;
    };

    class FunctionVisitor : public IVisitor
    {
    public:
        void accept(IAstFunction* fn_declaration) override;
    };

    class ImportVisitor : public IVisitor
    {
        std::string _current_file_name;
        std::map</* file_name = */ std::string, /* package_name = */ std::string> _file_package_mapping;
        std::map</* package_name = */ std::string, /* import_list = */ std::vector<std::string>> _import_registry;

    public:
        void set_current_file_name(const std::string& file_name)
        {
            this->_current_file_name = file_name;
        }

        void accept(AstImport* node);

        void accept(AstPackage* node);
    };
}
