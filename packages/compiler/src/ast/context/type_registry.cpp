#include "errors.h"
#include "ast/casting.h"
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
        if (current->get_context_type() != ContextType::GLOBAL &&
            current->get_context_type() != ContextType::MODULE)
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

/// Gets the root struct type layout for <code>name</code>.
/// Will recursively look up the parent struct definition if <code>name</code> is a reference struct type.
std::optional<AstStructType*> ParsingContext::get_struct_type(const std::string& name) const
{
    const auto type_def = get_type_definition(name);

    if (!type_def.has_value())
    {
        return std::nullopt;
    }

    // It's an immediate struct definition (type K = { ... }),
    // so we're safe
    if (const auto struct_ty = cast_type<AstStructType*>(type_def.value()->get_type()))
    {
        return struct_ty;
    }

    // It might be a reference struct, so we'll have to recursively extract it here.
    // e.g.,
    // type A = { ... };
    // type B = A; (named type)
    const auto* named_type = cast_type<AstNamedType*>(type_def.value()->get_type());
    int recursion_depth = 0;

    while (named_type != nullptr)
    {
        auto reference_type_def = get_type_definition(named_type->get_name());

        if (!reference_type_def.has_value())
        {
            return std::nullopt;
        }

        if (const auto struct_ty = cast_type<AstStructType
            *>(reference_type_def.value()->get_type()))
        {
            return struct_ty;
        }

        named_type = cast_type<AstNamedType*>(reference_type_def.value()->get_type());

        if (++recursion_depth > MAX_RECURSION_DEPTH)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Exceeded maximum recursion depth of {} while resolving struct type '{}'", MAX_RECURSION_DEPTH, name),
                {
                    ErrorSourceReference(
                        "Type definition here",
                        type_def.value()->get_type()->get_source_fragment()
                    )
                }
            );
        }
    }

    return std::nullopt;
}

void ParsingContext::define_type(const Symbol& type_name, std::unique_ptr<IAstType> type) const
{
    auto& root_context = const_cast<ParsingContext&>(this->traverse_to_root());

    if (const auto existing_def = root_context.get_type_definition(type_name.internal_name);
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

    root_context._symbols.push_back(std::make_unique<TypeDef>(type_name, std::move(type)));
}
