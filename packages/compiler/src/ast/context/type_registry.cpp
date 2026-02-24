#include "ast/parsing_context.h"

#include <ranges>

using namespace stride::ast;

std::vector<std::pair<std::string, IAstType*>> definition::TypeDef::get_struct_type_fields() const
{
    std::vector<std::pair<std::string, IAstType*>> copy{};
    copy.reserve(this->_fields.size());

    for (const auto& [name, type] : this->_fields)
    {
        copy.emplace_back(name, type.get());
    }

    return std::move(copy);
}

void ParsingContext::define_struct_type_member(
    const std::string& struct_name,
    const std::string& member_name,
    std::unique_ptr<IAstType> type
) const
{
    const auto& struct_definition = get_struct_def(struct_name);
    if (!struct_definition.has_value())
    {
        throw parsing_error(
            std::format("Struct '{}' is not defined in this scope", struct_name)
        );
    }

    struct_definition.value()->define_member(member_name, std::move(type));
}

std::optional<std::vector<std::pair<std::string, IAstType*>>> ParsingContext::get_struct_type_fields(
    const std::string& name
) const
{
    auto definition = get_struct_def(name);

    if (!definition)
    {
        return std::nullopt;
    }

    while (definition.value()->is_reference_struct())
    {
        definition = get_struct_def(definition.value()->get_reference_struct().value().name);

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

void definition::TypeDef::define_member(const std::string& member_name, std::unique_ptr<IAstType> type)
{
    for (const auto& key : this->_fields | std::views::keys)
    {
        if (key == member_name)
        {
            throw parsing_error(
                "Struct member already defined in scope"
            );
        }
    }

    this->_fields.emplace_back(member_name, std::move(type));
}

std::optional<IAstType*> definition::TypeDef::get_struct_member_field_type(
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
std::optional<IAstType*> definition::TypeDef::get_struct_member_field_type(
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

void ParsingContext::define_type(const Symbol& type_name, std::unique_ptr<IAstType> type) {}
