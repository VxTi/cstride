#include "../../../include/ast/traversal.h"

#include "ast/ast.h"
#include "ast/casting.h"
#include "ast/symbol_table.h"
#include "ast/visitor.h"
#include "ast/nodes/ast_node.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/package.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/type_definition.h"
#include "ast/nodes/while_loop.h"

#include <ranges>

using namespace stride::ast;

#define CONTEXT_NAME_DELIMITER ("::")

void AstNodeTraverser::traverse(IVisitor* visitor, const AstBranch* branch)
{
    visitor->context_name = "";
    visitor->current_context_type = ContextType::GLOBAL;
    visitor->current_symbol_table = branch->get_symbol_table();
    visitor->symbol_table_stack.clear();

    this->visit(visitor, branch->get_node());
}

void AstNodeTraverser::start_block_visitation(IVisitor* visitor, AstBlock* block)
{
    if (block->get_symbol_table() == nullptr)
    {
        if (visitor->current_context_type == ContextType::GLOBAL)
        {
            block->set_symbol_table(visitor->current_symbol_table);
        }
        else
        {
            block->set_symbol_table(
                std::make_shared<SymbolTable>(visitor->context_name, visitor->current_context_type, visitor->current_symbol_table)
            );
        }
    }
    visitor->symbol_table_stack.push_back(visitor->current_symbol_table);
    visitor->current_symbol_table = block->get_symbol_table();
}

void AstNodeTraverser::end_block_visitation(IVisitor* visitor)
{
    if (visitor->symbol_table_stack.empty())
    {
        visitor->context_name = "";
        visitor->current_context_type = ContextType::GLOBAL;
        return;
    }

    visitor->current_symbol_table = visitor->symbol_table_stack.back();
    visitor->symbol_table_stack.pop_back();
    visitor->context_name = visitor->current_symbol_table->get_scope_name();
    visitor->current_context_type = visitor->current_symbol_table->get_context_type();
}

void AstNodeTraverser::visit_block(IVisitor* visitor, const AstBlock* node)
{
    for (const auto& child : node->get_children())
    {
        visit(visitor, child.get());
    }
}

void AstNodeTraverser::visit_expression(IVisitor* visitor, IAstExpression* node)
{
    if (!node)
        return;

    if (const auto* binary = cast_expr<IBinaryOp*>(node))
    {
        visit_expression(visitor, binary->get_left());
        visit_expression(visitor, binary->get_right());
    }
    else if (const auto* unary = cast_expr<AstUnaryOp*>(node))
    {
        visit_expression(visitor, unary->get_operand());
    }
    else if (const auto* var_decl = cast_expr<AstVariableDeclaration*>(node))
    {
        if (var_decl->get_initial_value())
            visit_expression(visitor, var_decl->get_initial_value());
    }
    else if (auto* fn_call = cast_expr<AstFunctionCall*>(node))
    {
        for (const auto& arg : fn_call->get_arguments())
            visit_expression(visitor, arg.get());

        visitor->accept_function_call_node(visitor->current_symbol_table.get(), fn_call);
    }
    else if (const auto* array = cast_expr<AstArray*>(node))
    {
        for (const auto& elem : array->get_elements())
            visit_expression(visitor, elem.get());
    }
    else if (const auto* array_accessor = cast_expr<AstArrayMemberAccessor*>(node))
    {
        visit_expression(visitor, array_accessor->get_array_base());
        visit_expression(visitor, array_accessor->get_index());
    }
    else if (const auto* struct_init = cast_expr<AstObjectInitializer*>(node))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
            visit_expression(visitor, val.get());
    }
    else if (const auto* tuple_init = cast_expr<AstTupleInitializer*>(node))
    {
        for (const auto& member : tuple_init->get_members())
            visit_expression(visitor, member.get());
    }
    else if (const auto* reassign = cast_expr<AstVariableReassignment*>(node))
    {
        visit_expression(visitor, reassign->get_identifier());
        visit_expression(visitor, reassign->get_value());
    }
    else if (const auto* chained = cast_expr<AstChainedExpression*>(node))
    {
        // Visit the base expression so its type is resolved before the accessor's type is inferred.
        visit_expression(visitor, chained->get_base());
    }
    else if (auto* function_node = cast_expr<IAstFunction*>(node))
    {
        visitor->current_context_type = ContextType::FUNCTION;
        visitor->current_symbol_table->set_current_function(function_node);

        // If the function has generic instantiations, we wish to visit those nodes, rather than
        // the function itself. This way, we can properly resolve the types within the function.
        if (!function_node->get_generic_instantiations().empty())
        {
            for (const auto& generic_instantiation : function_node->get_generic_instantiations())
            {
                start_block_visitation(visitor, generic_instantiation->get_body());

                visitor->accept_function_node(
                    visitor->current_symbol_table.get(),
                    generic_instantiation.get()
                );
                visit_block(visitor, generic_instantiation->get_body());

                end_block_visitation(visitor);
            }
        }
        else
        {
            start_block_visitation(visitor, function_node->get_body());

            visitor->accept_function_node(visitor->current_symbol_table.get(), function_node);
            // We don't want to resolve the function body node if the function is generic, since it may
            // contain uninstantiated generic parameters that would fail to resolve.
            if (!function_node->is_generic())
            {
                visit_block(visitor, function_node->get_body());
            }

            end_block_visitation(visitor);
        }
    }
    else if (const auto* type_cast = cast_expr<AstTypeCastOp*>(node))
    {
        visit_expression(visitor, type_cast->get_value());
    }
    else if (const auto* indirect_call = cast_expr<AstIndirectCall*>(node))
    {
        for (const auto& arg : indirect_call->get_args())
        {
            visit_expression(visitor, arg.get());
        }
        visit_expression(visitor, indirect_call->get_callee());
    }

    visitor->accept_expression(visitor->current_symbol_table.get(), node);
    visitor->accept(visitor->current_symbol_table.get(), node);
}

void AstNodeTraverser::visit(IVisitor* visitor, IAstNode* node)
{
    if (!node)
        return;

    // Mainly statement parsing here

    if (auto* conditional = dynamic_cast<AstConditionalStatement*>(node))
    {
        visit(visitor, conditional->get_condition());
        visit(visitor, conditional->get_body());

        if (conditional->get_else_body())
            visit(visitor, conditional->get_else_body());
    }
    else if (auto* while_loop = dynamic_cast<AstWhileLoop*>(node))
    {
        visitor->current_context_type = ContextType::CONTROL_FLOW;
        if (while_loop->get_condition())
            visit(visitor, while_loop->get_condition());

        visit(visitor, while_loop->get_body());
    }
    else if (auto* for_loop = dynamic_cast<AstForLoop*>(node))
    {
        visitor->current_context_type = ContextType::CONTROL_FLOW;

        start_block_visitation(visitor, for_loop->get_body());

        if (for_loop->get_initializer())
            visit(visitor, for_loop->get_initializer());

        if (for_loop->get_condition())
            visit(visitor, for_loop->get_condition());

        if (for_loop->get_incrementor())
            visit(visitor, for_loop->get_incrementor());

        visit_block(visitor, for_loop->get_body());

        end_block_visitation(visitor);
    }
    else if (const auto* return_stmt = dynamic_cast<AstReturnStatement*>(node))
    {
        if (return_stmt->get_return_expression().has_value())
            visit(visitor, return_stmt->get_return_expression().value().get());
    }
    else if (auto* import = dynamic_cast<AstImport*>(node))
    {
        visitor->accept_import_node(visitor->current_symbol_table.get(), import);
    }
    else if (auto* package = dynamic_cast<AstPackage*>(node))
    {
        visitor->accept_package_node(visitor->current_symbol_table.get(), package);
    }
    else if (auto* type_def = dynamic_cast<AstTypeDefinition*>(node))
    {
        visitor->accept_type_definition_node(visitor->current_symbol_table.get(), type_def);
    }
    else if (auto* module = dynamic_cast<AstModule*>(node))
    {
        const auto& previous_context_name = visitor->context_name;

        visitor->current_context_type = ContextType::MODULE;
        visitor->context_name = join(
            { previous_context_name, module->get_name() },
            CONTEXT_NAME_DELIMITER
        );
        visit(visitor, module->get_body());
    }
    else if (auto* block = dynamic_cast<AstBlock*>(node))
    {
        start_block_visitation(visitor, block);

        visit_block(visitor, block);

        end_block_visitation(visitor);
    }
    else if (auto* expr = dynamic_cast<IAstExpression*>(node))
    {
        visit_expression(visitor, expr);
        return;
    }

    visitor->accept(visitor->current_symbol_table.get(), node);
}
