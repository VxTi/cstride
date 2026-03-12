#include "errors.h"
#include "ast/ast.h"
#include "ast/visitor.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/import.h"
#include "ast/nodes/package.h"

void stride::ast::ImportVisitor::accept(AstImport* node)
{
    const auto& [package_name, module_import_symbols] = node->get_dependency();
    std::vector<std::string> submodules;
    submodules.reserve(module_import_symbols.size());

    for (const auto& import_symbol : module_import_symbols)
    {
        submodules.push_back(import_symbol.internal_name);
    }

    if (this->_import_registry.contains(this->_current_file_name))
    {
        // Aggregate existing imports
        auto& package_imports = this->_import_registry.at(this->_current_file_name);
        package_imports
           .at(package_name.name)
           .insert(
                package_imports.at(package_name.name).end(),
                submodules.begin(),
                submodules.end()
            );
    }
    // If the import list for this file isn't yet constructed, we create a
    // new list of pairs and just return
    else
    {
        std::map<std::string, std::vector<std::string>> package_imports;
        package_imports.emplace(package_name.name, std::move(submodules));
        this->_import_registry.emplace(this->_current_file_name, package_imports);
    }
}

void stride::ast::ImportVisitor::accept(AstPackage* node)
{
    this->_package_file_mapping[node->get_package_name()].push_back(this->_current_file_name);
}

void stride::ast::ImportVisitor::cross_register_symbols(Ast* ast) const
{
    for (const auto& [file_name, node] : ast->get_files())
    {
        if (!this->_import_registry.contains(file_name))
            continue;

        // Get required imports by file_name
        for (const auto imports = this->_import_registry.at(file_name);
             const auto& [package_name, import_names] : imports)
        {
            if (!this->_package_file_mapping.contains(package_name))
            {
                throw parsing_error(
                    ErrorType::REFERENCE_ERROR,
                    std::format("Package '{}' not found", package_name),
                    node->get_source_fragment()
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
                    const auto& node_with_exports = ast->get_files().at(file_name_with_exports);
                    definition = node_with_exports->get_context()->get_definition_by_internal_name(import_name);
                    if (definition.has_value())
                        break;
                }

                if (!definition.has_value())
                {
                    throw parsing_error(
                        ErrorType::REFERENCE_ERROR,
                        std::format("Variable or function '{}' not found in package '{}'", import_name, package_name),
                        node->get_source_fragment()
                    );
                }

                if (definition.value()->get_visibility() != VisibilityModifier::PUBLIC)
                {
                    throw parsing_error(
                        ErrorType::REFERENCE_ERROR,
                        std::format("Variable or function '{}' is not public", import_name),
                        node->get_source_fragment()
                    );
                }

                // Define only if not yet present
                if (node->get_context()->get_definition_by_internal_name(definition.value()->get_internal_symbol_name())
                    == std::nullopt)
                {
                    node->get_context()->define(std::move(definition.value()));
                }
            }
        }
    }
}
