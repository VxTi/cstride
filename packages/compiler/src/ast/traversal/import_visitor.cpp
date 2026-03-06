#include "ast/ast.h"
#include "ast/visitor.h"
#include "ast/nodes/import.h"
#include "ast/nodes/package.h"

void stride::ast::ImportVisitor::accept(AstImport* node)
{
    const auto& [package_name, module_import_symbols] = node->get_dependency();
    std::vector<std::string> submodules;
    submodules.reserve(submodules.size());

    for (const auto& import_symbol : module_import_symbols)
    {
        submodules.push_back(import_symbol.name);
    }

    this->_import_registry.emplace(
        this->_current_file_name,
        std::make_pair(
            package_name.name,
            std::move(submodules)
        )
    );
}

void stride::ast::ImportVisitor::accept(AstPackage* node)
{
    this->_file_package_mapping.emplace(this->_current_file_name, node->get_package_name());
}

void stride::ast::ImportVisitor::cross_register_symbols(Ast* ast)
{

}
