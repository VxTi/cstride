#include "errors.h"
#include "ast/ast.h"
#include "ast/visitor.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/import.h"
#include "ast/nodes/package.h"

void stride::ast::ImportVisitor::accept_import_node(SymbolTable* symbol_table, AstImport* node)
{
    const auto& package_identifier = node->get_package_identifier();
    const auto& import_identifiers = node->get_import_list();

    const auto& pkg_name = package_identifier->get_scoped_name();

    std::vector<std::string> import_list;
    import_list.reserve(import_identifiers.size());

    for (const auto& import_symbol : import_identifiers)
    {
        import_list.push_back(import_symbol->get_scoped_name());
    }

    if (this->_import_registry.contains(this->_current_file_name))
    {
        // Aggregate existing imports
        auto& package_imports = this->_import_registry.at(this->_current_file_name);
        package_imports
           .at(pkg_name)
           .insert(
                package_imports.at(pkg_name).end(),
                import_list.begin(),
                import_list.end()
            );
    }
    // If the import list for this file isn't yet constructed, we create a
    // new list of pairs and just return
    else
    {
        std::map<std::string, std::vector<std::string>> package_imports;
        package_imports.emplace(pkg_name, std::move(import_list));
        this->_import_registry.emplace(this->_current_file_name, package_imports);
    }
}

void stride::ast::ImportVisitor::accept_package_node(SymbolTable* symbol_table, AstPackage* node)
{
    this->_package_file_mapping[node->get_package_name()].push_back(this->_current_file_name);
}

void stride::ast::ImportVisitor::cross_register_symbols(Ast* ast) const
{
    for (const auto& [file_path, branch] : ast->get_branches())
    {
        if (!this->_import_registry.contains(file_path))
            continue;

        // Get required imports by file_name
        for (const auto imports = this->_import_registry.at(file_path);
             const auto& [package_name, import_names] : imports)
        {
            if (!this->_package_file_mapping.contains(package_name))
            {
                throw stride_error(
                    ErrorType::REFERENCE_ERROR,
                    std::format("Package '{}' not found", package_name),
                    branch->get_node()->get_source_position()
                );
            }

            // The Ast nodes from which we wish to extract the symbols
            const auto& files_with_exports = this->_package_file_mapping.at(package_name);

            // Acquire all symbols from the package's files
            for (const auto& import_name : import_names)
            {
                // Search across all files that belong to this package
                std::optional<std::unique_ptr<definition::IDefinition>> definition;
                for (const auto& file_name_with_exports : files_with_exports)
                {
                    const auto& node_with_exports = ast->get_branches().at(file_name_with_exports);

                    definition = node_with_exports->get_node()->get_symbol_table()->get_definition_by_internal_name(import_name);
                    if (definition.has_value())
                        break;
                }

                if (!definition.has_value())
                {
                    throw stride_error(
                        ErrorType::REFERENCE_ERROR,
                        std::format("Variable or function '{}' not found in package '{}'", import_name, package_name),
                        branch->get_node()->get_source_position()
                    );
                }

                if (definition.value()->get_visibility() != VisibilityModifier::PUBLIC)
                {
                    throw stride_error(
                        ErrorType::REFERENCE_ERROR,
                        std::format("Variable or function '{}' is not public", import_name),
                        branch->get_node()->get_source_position()
                    );
                }

                // Define only if not yet present
                if (branch->get_node()->get_symbol_table()->get_definition_by_internal_name(definition.value()->get_internal_symbol_name())
                    == std::nullopt)
                {
                    branch->get_node()->get_symbol_table()->define(std::move(definition.value()));
                }
            }
        }
    }
}
