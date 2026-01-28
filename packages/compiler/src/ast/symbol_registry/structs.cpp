#include "ast/symbol_registry.h"

using namespace stride::ast;

const StructSymbolDef* SymbolRegistry::get_struct_def(const std::string& name) const
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
            if (const auto* struct_def = dynamic_cast<const StructSymbolDef*>(definition.get()))
            {
                if (struct_def->get_internal_symbol_name() == name)
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
    std::string internal_name,
    std::vector<std::unique_ptr<IAstInternalFieldType>> fields
) const
{
    auto& root = const_cast<SymbolRegistry&>(this->traverse_to_root());
    root.symbols.push_back(std::make_unique<StructSymbolDef>(
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