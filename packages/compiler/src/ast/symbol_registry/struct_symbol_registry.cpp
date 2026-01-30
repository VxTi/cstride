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
    std::vector<std::pair<std::string, std::unique_ptr<IAstInternalFieldType>>> fields
) const
{
    if (const auto existing_def = this->get_struct_def(struct_name); existing_def != nullptr)
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Struct '{}' is already defined in this scope", struct_name),
            *fields.begin()->second->get_source(),
            fields.begin()->second->get_source_position()
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
    std::string struct_name,
    std::string internal_name,
    std::string reference_struct_name
) const
{
    auto& root = const_cast<SymbolRegistry&>(this->traverse_to_root());
    root.symbols.push_back(std::make_unique<StructSymbolDef>(
        std::move(struct_name),
        std::move(internal_name),
        std::move(reference_struct_name)
    ));
}

std::vector<std::pair<std::string, IAstInternalFieldType*>> StructSymbolDef::get_fields() const
{
    std::vector<std::pair<std::string, IAstInternalFieldType*>> copy{};
    copy.reserve(this->_fields.size());

    for (const auto& [name, type] : this->_fields)
    {
        copy.push_back({name, type.get()});
    }

    return std::move(copy);
}

IAstInternalFieldType* StructSymbolDef::get_field_type(const std::string& field_name) const
{
    for (const auto& [name, type] : this->_fields)
    {
        if (name == field_name)
        {
            return type.get();
        }
    }
    return nullptr;
}

bool StructSymbolDef::is_reference_struct() const
{
    return _reference_struct_name.has_value();
}

std::optional<int> StructSymbolDef::get_member_index(const std::string& member_name) const
{
    for (size_t i = 0; i < this->_fields.size(); i++)
    {
        if (this->_fields[i].first == member_name)
        {
            return static_cast<int>(i);
        }
    }
    return std::nullopt;
}