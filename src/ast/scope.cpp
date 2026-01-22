#include "ast/scope.h"
#include "errors.h"

using namespace stride::ast;

SymbolFnDefinition::~SymbolFnDefinition() = default;

std::shared_ptr<Scope> Scope::traverse_to_root()
{
    auto* global_scope = this;
    while (global_scope != nullptr && global_scope->parent_scope != nullptr)
    {
        global_scope = global_scope->parent_scope.get();
    }
    return std::shared_ptr<Scope>(global_scope, [](Scope*) {});
}

bool Scope::is_variable_defined_in_scope(const std::string& variable_name) const
{
    for (const auto& symbol_def : this->symbols)
    {
        if (const auto* var_def = dynamic_cast<FieldSymbolDef*>(symbol_def.get()))
        {
            if (var_def->get_internal_symbol_name() == variable_name)
            {
                return true;
            }
        }
    }
    return false;
}

bool Scope::is_variable_defined_globally(const std::string& variable_name) const
{
    auto current = this;
    while (current != nullptr)
    {
        if (current->is_variable_defined_in_scope(variable_name))
        {
            return true;
        }
        current = current->parent_scope.get();
    }
    return false;
}

bool Scope::is_function_defined_globally(const std::string& internal_function_name)
{
    for (const auto root = this->traverse_to_root();
         const auto& symbol : root->symbols)
    {
        if (const auto* fn_def = dynamic_cast<SymbolFnDefinition*>(symbol.get()))
        {
            if (fn_def->get_internal_symbol_name() == internal_function_name)
            {
                return true;
            }
        }
    }

    return false;
}

bool Scope::is_symbol_type_defined_globally(const std::string& symbol_name, const IdentifiableSymbolType& type)
{
    for (const auto root = this->traverse_to_root();
         const auto& symbol : root->symbols)
    {
        if (const auto* identifiable = dynamic_cast<IdentifiableSymbolDef*>(symbol.get()))
        {
            if (identifiable->get_symbol_type() == type
                && identifiable->get_internal_symbol_name() == symbol_name)
            {
                return true;
            }
        }
    }

    return false;
}

void Scope::define_function(
    const std::string& internal_function_name,
    std::vector<std::shared_ptr<IAstInternalFieldType>> parameter_types,
    std::shared_ptr<IAstInternalFieldType> return_type
)
{
    const auto global_scope = this->traverse_to_root();
    global_scope->symbols.push_back(std::make_unique<SymbolFnDefinition>(
        std::move(parameter_types),
        std::move(return_type),
        internal_function_name
    ));
}

void Scope::define_field(
    const std::string& field_name,
    const std::string& internal_name,
    const std::shared_ptr<IAstInternalFieldType>& type,
    const int flags
)
{
    if (is_variable_defined_in_scope(field_name))
    {
        throw parsing_error(
            make_ast_error(
                *type->source,
                type->source_offset,
                "Variable '" + field_name + "' is already defined in this scope"
            )
        );
    }

    this->symbols.push_back(std::make_unique<FieldSymbolDef>(
        field_name,
        internal_name,
        type,
        flags
    ));
}

void Scope::define_symbol(const std::string& symbol_name, const IdentifiableSymbolType type)
{
    const auto global_scope = this->traverse_to_root();
    global_scope->symbols.push_back(std::make_unique<IdentifiableSymbolDef>(
        type,
        symbol_name
    ));
}


FieldSymbolDef* Scope::get_variable_def(const std::string& variable_name) const
{
    for (const auto& symbol_def : this->symbols)
    {
        if (auto* var_def = dynamic_cast<FieldSymbolDef*>(symbol_def.get()))
        {
            if (var_def->get_variable_name() == variable_name
                || var_def->get_internal_symbol_name() == variable_name)
            {
                return var_def;
            }
        }
    }
    return nullptr;
}

IdentifiableSymbolDef* Scope::get_symbol_def(const std::string& symbol_name) const
{
    for (const auto& symbol_def : this->symbols)
    {
        if (auto* identif_def = dynamic_cast<IdentifiableSymbolDef*>(symbol_def.get()))
        {
            if (identif_def->get_internal_symbol_name() == symbol_name)
            {
                return identif_def;
            }
        }
    }
    return nullptr;
}

SymbolFnDefinition* Scope::get_function_def(const std::string& function_name) const
{
    for (const auto& symbol_def : this->symbols)
    {
        if (auto* fn_def = dynamic_cast<SymbolFnDefinition*>(symbol_def.get()))
        {
            if (fn_def->get_internal_symbol_name() == function_name)
            {
                return fn_def;
            }
        }
    }
    return nullptr;
}
