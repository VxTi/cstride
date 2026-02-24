#include "ast/parsing_context.h"

#include <ranges>

using namespace stride::ast;
using namespace stride::ast::definition;

std::vector<std::pair<std::string, IAstType*>> TypeDef::get_struct_type_fields() const
{
    std::vector<std::pair<std::string, IAstType*>> copy{};
    copy.reserve(this->_fields.size());

    for (const auto& [name, type] : this->_fields)
    {
        copy.emplace_back(name, type.get());
    }

    return std::move(copy);
}

std::optional<std::vector<std::pair<std::string, IAstType*>>> ParsingContext::get_struct_type_fields(
    const std::string& name
) const
{
    auto definition = get_type_definition(name);

    if (!definition)
    {
        return std::nullopt;
    }

    while (definition.value()->is_reference_struct())
    {
        definition = get_type_definition(definition.value()->get_reference_struct().value().name);

        if (!definition)
        {
            return std::nullopt;
        }
    }

    return definition.value()->get_struct_type_fields();
}

// It's a reference struct, if the type is a named type
bool definition::TypeDef::is_reference_struct() const
{
    if (cast_type<AstNamedType *>(this->_type.get()))
    {
        return true;
    }
    return false;
}

std::optional<IAstType*> TypeDef::get_struct_member_field_type(
    const std::string& field_name,
    const std::vector<std::pair<std::string, IAstType*>>& fields)
{
    for (const auto& [name, type] : fields)
    {
        if (name == field_name)
        {
            return type;
        }
    }
    return std::nullopt;
}

// Note that if this struct is a reference struct, this will return nullopt
std::optional<IAstType*> TypeDef::get_struct_member_field_type(
    const std::string& field_name)
{
    for (const auto& [name, type] : this->_fields)
    {
        if (name == field_name)
        {
            return type.get();
        }
    }
    return std::nullopt;
}


std::optional<int> definition::TypeDef::get_struct_field_member_index(
    const std::string& member_name) const
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

std::optional<definition::TypeDef*> ParsingContext::get_type_definition(
    const std::string& name) const
{
    auto current = this;

    while (current != nullptr)
    {
        if (current->get_current_scope_type() != ScopeType::GLOBAL &&
            current->get_current_scope_type() != ScopeType::MODULE)
        {
            current = current->_parent_registry.get();
            continue;
        }

        for (const auto& definition : current->_symbols)
        {
            if (auto* struct_def = dynamic_cast<TypeDef*>(definition.get()))
            {
                if (struct_def->get_type_variant() != TypeDefVariant::STRUCT ||
                    (struct_def->get_type_variant() == TypeDefVariant::TYPE_ALIAS && cast_type<>(struct_def->get_type())))

                    // Here we don't check for the internal name, as we don't always know what the data
                        // layout is initially (which is used for resolving the actual internal name)
                            if ( struct_def->get_internal_symbol_name() == name)
                            {
                                return struct_def;
                            }
            }
        }

        current = current->_parent_registry.get();
    }

    return std::nullopt;
}

void ParsingContext::define_type(const Symbol& type_name, std::unique_ptr<IAstType> type) {}
