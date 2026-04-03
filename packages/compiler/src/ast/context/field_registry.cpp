#include "errors.h"
#include "ast/symbol_table.h"

#include <algorithm>

using namespace stride::ast;

definition::FieldDefinition* SymbolTable::get_variable_definition(
    const std::string& variable_name,
    const bool is_internal_name
) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (auto* field_definition = dynamic_cast<definition::FieldDefinition*>(symbol_def.get()))
        {
            if (field_definition->get_internal_symbol_name() == variable_name ||
                (is_internal_name && field_definition->get_field_name() == variable_name))
            {
                return field_definition;
            }
        }
    }
    return nullptr;
}

bool SymbolTable::is_field_defined_in_scope(const std::string& variable_name) const
{
    return std::ranges::any_of(
        this->_symbols,
        [&](const auto& symbol_def)
        {
            if (const auto* var_def = dynamic_cast<const definition::FieldDefinition*>(symbol_def.
                get()))
            {
                return var_def->get_internal_symbol_name() == variable_name;
            }
            return false;
        });
}

void SymbolTable::define_variable(Symbol variable_symbol, const VisibilityModifier visibility, const int flags)
{
    define_variable(std::move(variable_symbol), nullptr, visibility, flags);
}

void SymbolTable::define_variable(
    Symbol variable_symbol,
    std::unique_ptr<IAstType> type,
    VisibilityModifier visibility,
    int flags
)
{
    if (is_field_defined_in_scope(variable_symbol.internal_name))
    {
        throw stride_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is already defined in this scope", variable_symbol.name),
            variable_symbol.symbol_position
        );
    }

    printf("defining variable \"%s\"\n", variable_symbol.name.c_str());

    this->_symbols.push_back(
        std::make_unique<definition::FieldDefinition>(
            std::move(variable_symbol),
            std::move(type),
            visibility,
            flags
        )
    );
}

void SymbolTable::set_variable_type(Symbol variable_symbol, std::unique_ptr<IAstType> type) const
{
    const auto definition = get_variable_definition(variable_symbol.internal_name, false);

    if (!definition)
    {
        throw stride_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is not defined", variable_symbol.name),
            variable_symbol.symbol_position
        );
    }

    definition->set_type(std::move(type));
}

definition::FieldDefinition* SymbolTable::lookup_variable(const std::string& name, const bool use_raw_name) const
{
    auto current = this;
    while (current != nullptr)
    {
        if (const auto def = current->get_variable_definition(name, use_raw_name))
        {
            return def;
        }
        current = current->_parent_registry.get();
    }
    return nullptr;
}
