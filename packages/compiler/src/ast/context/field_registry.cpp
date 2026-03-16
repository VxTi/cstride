#include "errors.h"
#include "ast/parsing_context.h"

#include <algorithm>

using namespace stride::ast;

definition::FieldDefinition* ParsingContext::get_variable_def(
    const std::string& variable_name,
    const bool use_raw_name
) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (auto* field_definition = dynamic_cast<definition::FieldDefinition*>(
            symbol_def.get()))
        {
            if (field_definition->get_internal_symbol_name() == variable_name
                || (use_raw_name && field_definition->get_field_name() == variable_name))
            {
                return field_definition;
            }
        }
    }
    return nullptr;
}

bool ParsingContext::is_field_defined_in_scope(
    const std::string& variable_name) const
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

bool ParsingContext::is_field_defined_globally(
    const std::string& field_name) const
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

void ParsingContext::define_variable_globally(
    Symbol variable_symbol,
    std::unique_ptr<IAstType> type,
    VisibilityModifier visibility,
    const bool overwrite
) const
{
    if (is_field_defined_globally(variable_symbol.internal_name))
    {
        if (overwrite)
        {
            for (auto& symbol_def : this->_symbols)
            {
                if (auto* var_def = dynamic_cast<definition::FieldDefinition*>(symbol_def.get());
                    var_def != nullptr &&
                    var_def->get_internal_symbol_name() == variable_symbol.internal_name)
                {
                    var_def->set_type(std::move(type));
                    var_def->set_visibility(visibility);
                    return;
                }
            }
            return;
        }

        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is already defined in global scope", variable_symbol.name),
            type->get_source_fragment()
        );
    }

    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());
    global_scope._symbols.push_back(
        std::make_unique<definition::FieldDefinition>(
            std::move(variable_symbol),
            std::move(type),
            visibility
        )
    );
}

void ParsingContext::define_variable(
    Symbol variable_sym,
    std::unique_ptr<IAstType> type,
    VisibilityModifier visibility,
    const bool overwrite
)
{
    if (this->is_global_scope())
    {
        this->define_variable_globally(
            std::move(variable_sym),
            std::move(type),
            visibility,
            overwrite
        );
        return;
    }

    if (is_field_defined_in_scope(variable_sym.internal_name))
    {
        if (overwrite)
        {
            for (auto& symbol_def : this->_symbols)
            {
                if (auto* var_def = dynamic_cast<definition::FieldDefinition*>(symbol_def.get());
                    var_def != nullptr &&
                    var_def->get_internal_symbol_name() == variable_sym.internal_name)
                {
                    var_def->set_type(std::move(type));
                    var_def->set_visibility(visibility);
                    return;
                }
            }
            return;
        }

        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is already defined in this scope", variable_sym.name),
            type->get_source_fragment());
    }

    this->_symbols.push_back(
        std::make_unique<definition::FieldDefinition>(
            std::move(variable_sym),
            std::move(type),
            visibility
        )
    );
}

definition::FieldDefinition* ParsingContext::lookup_variable(
    const std::string& name,
    const bool use_raw_name
)
const
{
    auto current = this;
    while (current != nullptr)
    {
        if (auto def = current->get_variable_def(name, use_raw_name))
        {
            return def;
        }
        current = current->_parent_registry.get();
    }
    return nullptr;
}
