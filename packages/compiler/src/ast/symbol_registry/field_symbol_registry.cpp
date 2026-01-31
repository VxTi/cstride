#include "ast/symbol_registry.h"

using namespace stride::ast;

bool SymbolRegistry::is_field_defined_in_scope(const std::string& variable_name) const
{
    return std::ranges::any_of(this->_symbols, [&](const auto& symbol_def)
    {
        if (const auto* var_def = dynamic_cast<const FieldSymbolDef*>(symbol_def.get()))
        {
            return var_def->get_internal_symbol_name() == variable_name;
        }
        return false;
    });
}

bool SymbolRegistry::is_field_defined_globally(const std::string& field_name) const
{
    auto current = this;
    while (current != nullptr)
    {
        if (current->is_field_defined_in_scope(field_name))
        {
            return true;
        }
        current = current->_parent_registry.get();
    }
    return false;
}


void SymbolRegistry::define_field(
    std::string field_name,
    const std::string& internal_name,
    std::unique_ptr<IAstType> type
)
{
    if (is_field_defined_in_scope(field_name))
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Field '{}' is already defined in this scope", field_name),
            *type->get_source(),
            type->get_source_position()
        );
    }

    this->_symbols.push_back(
        std::make_unique<FieldSymbolDef>(
            std::move(field_name),
            std::move(internal_name),
            std::move(type)
        )
    );
}


const FieldSymbolDef* SymbolRegistry::field_lookup(const std::string& name) const
{
    auto current = this;
    while (current != nullptr)
    {
        if (const auto def = current->get_variable_def(name))
        {
            return def;
        }
        current = current->_parent_registry.get();
    }
    return nullptr;
}
