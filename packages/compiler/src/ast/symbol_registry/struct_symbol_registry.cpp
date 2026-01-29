#include "ast/symbol_registry.h"

using namespace stride::ast;

StructSymbolDef* SymbolRegistry::get_struct_def(const std::string& name) const
{
    auto current = this;

    while (current != nullptr)
    {
        if (current->get_current_scope() != ScopeType::GLOBAL && current->get_current_scope() != ScopeType::MODULE)
        {
            current = current->_parent_registry.get();
            continue;
        }

        for (const auto& definition : current->symbols)
        {
            if (auto* struct_def = dynamic_cast<StructSymbolDef*>(definition.get()))
            {
                // Here we don't check for the internal name, as we don't always know what the data
                // layout is initially (which is used for resolving the actual internal name)
                if (struct_def->get_name() == name)
                {
                    return struct_def;
                }
            }
        }

        current = current->_parent_registry.get();
    }

    return nullptr;
}

void SymbolRegistry::define_struct(
    std::string struct_name,
    std::string internal_name,
    std::unordered_map<std::string, std::unique_ptr<IAstInternalFieldType>> fields
) const
{
    if (const auto existing_def = this->get_struct_def(struct_name); existing_def != nullptr)
    {
        throw parsing_error(
            make_source_error(
                ErrorType::SEMANTIC_ERROR,
                std::format("Struct '{}' is already defined in this scope", struct_name),
                *fields.begin()->second->get_source(),
                fields.begin()->second->get_source_position()
            )
        );
    }

    auto& root = const_cast<SymbolRegistry&>(this->traverse_to_root());
    root.symbols.push_back(std::make_unique<StructSymbolDef>(
        std::move(struct_name),
        std::move(internal_name),
        std::move(fields)
    ));
}

void SymbolRegistry::define_struct(
    std::string internal_name,
    std::string reference_struct_name
) const
{
    auto& root = const_cast<SymbolRegistry&>(this->traverse_to_root());
    root.symbols.push_back(std::make_unique<StructSymbolDef>(
        std::move(internal_name),
        std::move(reference_struct_name)
    ));
}
