#include "errors.h"
#include "ast/casting.h"
#include "ast/symbol_table.h"

#include <ranges>

using namespace stride::ast;
using namespace stride::ast::definition;

bool SymbolTable::is_type_defined(const std::string& type_name) const
{
    return get_type_definition(type_name).has_value();
}

bool SymbolTable::is_object_type_defined(const std::string& struct_name) const
{
    const auto type_def = get_type_definition(struct_name);

    return type_def.has_value() &&
        cast_type<AstObjectType*>(type_def.value()->get_type()) != nullptr;
}

std::optional<TypeDefinition*> SymbolTable::get_type_definition(const std::string& type_name) const
{
    auto current = this;

    while (current != nullptr)
    {
        for (const auto& type_definition : current->_type_definitions)
        {
            if (type_definition->get_type_name_symbol().internal_name == type_name)
            {
                return type_definition.get();
            }
        }

        current = current->_parent_registry.get();
    }

    return std::nullopt;
}

/// Gets the root struct type layout for <code>name</code>.
/// Will recursively look up the parent struct definition if <code>name</code> is a reference struct type.
std::optional<AstObjectType*> SymbolTable::get_object_type(const std::string& name) const
{
    const auto type_def = get_type_definition(name);

    if (!type_def.has_value())
    {
        return std::nullopt;
    }

    // It's an immediate struct definition (type K = { ... }),
    // so we're safe
    if (const auto object_type = cast_type<AstObjectType*>(type_def.value()->get_type()))
    {
        return object_type;
    }

    // It might be a reference struct, so we'll have to recursively extract it here.
    // e.g.,
    // type A = { ... };
    // type B = A; (named type)
    const auto* named_type = cast_type<AstAliasType*>(type_def.value()->get_type());
    int recursion_depth = 0;

    while (named_type != nullptr)
    {
        auto reference_type_def = get_type_definition(named_type->get_name());

        if (!reference_type_def.has_value())
        {
            return std::nullopt;
        }

        if (const auto object_type = cast_type<AstObjectType*>(reference_type_def.value()->get_type()))
        {
            return object_type;
        }

        named_type = cast_type<AstAliasType*>(reference_type_def.value()->get_type());

        if (++recursion_depth > MAX_RECURSION_DEPTH)
        {
            throw stride_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Exceeded maximum recursion depth of {} while resolving struct type '{}'",
                            MAX_RECURSION_DEPTH,
                            name),
                {
                    ErrorSourceReference(
                        "Type definition here",
                        type_def.value()->get_type()->get_source_position()
                    )
                }
            );
        }
    }

    return std::nullopt;
}

void SymbolTable::define_generic_type_alias(const Symbol& type_name, std::unique_ptr<IAstType> type)
{
    if (const auto existing_def = this->get_type_definition(type_name.internal_name);
        existing_def.has_value())
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Type '{}' is already defined in this scope", type_name.name),
            {
                ErrorSourceReference(
                    "Previous definition here",
                    existing_def.value()->get_type()->get_source_position()
                )
            }
        );
    }

    printf("Defining generic type alias \"%s\" to \"%s\"\n", type_name.name.c_str(), type->get_type_name().c_str());

    this->_type_definitions.push_back(
        std::make_unique<TypeDefinition>(
            type_name,
            std::move(type),
            EMPTY_GENERIC_PARAMETER_LIST,
            VisibilityModifier::PRIVATE
        )
    );
}

void SymbolTable::define_type(
    const Symbol& type_name,
    std::unique_ptr<IAstType> type,
    GenericParameterList generic_parameter_names,
    const VisibilityModifier visibility
)
{
    const auto& root_context = this->traverse_to_root();

    if (const auto existing_def = root_context->get_type_definition(type_name.internal_name);
        existing_def.has_value())
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Type '{}' is already defined in this scope", type_name.name),
            {
                ErrorSourceReference(
                    "Previous definition here",
                    existing_def.value()->get_type()->get_source_position()
                )
            }
        );
    }

    printf("Defining type \"%s\"\n", type_name.name.c_str());

    root_context->_type_definitions.push_back(
        std::make_unique<TypeDefinition>(
            type_name,
            std::move(type),
            std::move(generic_parameter_names),
            visibility
        )
    );
}
