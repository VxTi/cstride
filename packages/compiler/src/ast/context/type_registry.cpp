#include "ast/parsing_context.h"

#include <ranges>

using namespace stride::ast;
using namespace stride::ast::definition;

bool ParsingContext::is_type_defined(const std::string& type_name) const
{
    return get_type_definition(type_name).has_value();
}

bool ParsingContext::is_struct_type_defined(const std::string& struct_name) const
{
    const auto type_def = get_type_definition(struct_name);

    return type_def.has_value() &&
        cast_type<AstStructType*>(type_def.value()->get_type()) != nullptr;
}

std::optional<TypeDef*> ParsingContext::get_type_definition(const std::string& name) const
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

        for (const auto& symbol_definition : current->_symbols)
        {
            if (auto* type_definition = dynamic_cast<TypeDef*>(symbol_definition.get()))
            {
                if (type_definition->get_internal_symbol_name() == name)
                {
                    return type_definition;
                }
            }
        }

        current = current->_parent_registry.get();
    }

    return std::nullopt;
}

void ParsingContext::define_type(const Symbol& type_name, std::unique_ptr<IAstType> type)
{
    if (const auto existing_def = this->get_type_definition(type_name.internal_name);
        existing_def.has_value())
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Type '{}' is already defined in this scope", type_name.name),
            {
                ErrorSourceReference(
                    "Previous definition here",
                    existing_def.value()->get_type()->get_source_fragment()
                )
            }
        );
    }

    this->_symbols.push_back(std::make_unique<TypeDef>(type_name, std::move(type)));
}
