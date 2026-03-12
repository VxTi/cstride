#pragma once

#include "nodes/traversal.h"

#include <map>
#include <string>
#include <vector>

namespace stride::ast
{
    class AstImport;
    class AstPackage;
    class AstFunctionDeclaration;
    class IAstExpression;
    class Ast;

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
        std::string _current_file_name; // temporary values
        std::map<
            std::string,             /* package_name */
            std::vector<std::string> /* file_names   */
        > _package_file_mapping;
        std::map<
            std::string, /* file_name */
            std::map<
                std::string,             /* package_name */
                std::vector<std::string> /* module_names::functions/types */
            >
        > _import_registry;

    public:
        void set_current_file_name(const std::string& file_name)
        {
            this->_current_file_name = file_name;
        }

        void accept(AstImport* node) override;

        void accept(AstPackage* node) override;

        void cross_register_symbols(Ast* ast) const;
    };
}
