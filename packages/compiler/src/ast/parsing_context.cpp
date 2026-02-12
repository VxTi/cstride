#include "ast/parsing_context.h"

#include <algorithm>
#include "errors.h"
#include "ast/symbols.h"

using namespace stride::ast;

std::string stride::ast::scope_type_to_str(const ScopeType& scope_type)
{
    switch (scope_type)
    {
    case ScopeType::GLOBAL: return "global";
    case ScopeType::FUNCTION: return "function";
    case ScopeType::CLASS: return "class";
    case ScopeType::BLOCK: return "block";
    case ScopeType::MODULE: return "module";
    }
    return "unknown";
}

const ParsingContext& ParsingContext::traverse_to_root() const
{
    auto current = this;
    while (current->_parent_registry)
    {
        current = current->_parent_registry.get();
    }
    return *current;
}

bool ParsingContext::is_function_defined_globally(const std::string& internal_function_name) const
{
    return std::ranges::any_of(this->traverse_to_root()._symbols, [&](const auto& symbol)
    {
        if (const auto* fn_def = dynamic_cast<const SymbolFnDefinition*>(symbol.get()))
        {
            if (fn_def->get_internal_symbol_name() == internal_function_name)
            {
                return true;
            }
        }
        return false;
    });
}

void ParsingContext::define_function(
    const Symbol& symbol,
    std::vector<std::unique_ptr<IAstType>> parameter_types,
    std::unique_ptr<IAstType> return_type
) const
{
    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());

    if (this->is_function_defined_globally(symbol.internal_name))
    {
        throw std::runtime_error(
            "Function already defined globally: " + symbol.name
        );
    }

    global_scope._symbols.push_back(
        std::make_unique<SymbolFnDefinition>(
            std::move(parameter_types),
            std::move(return_type),
            symbol
        )
    );
}

void ParsingContext::define_symbol(const Symbol& symbol_name, const SymbolType type)
{
    this->_symbols.push_back(std::make_unique<IdentifiableSymbolDef>(
        type,
        symbol_name
    ));
}

const FieldSymbolDef* ParsingContext::get_variable_def(const std::string& variable_name) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (const auto* field_definition = dynamic_cast<const FieldSymbolDef*>(symbol_def.get()))
        {
            if (field_definition->get_internal_symbol_name() == variable_name)
            {
                return field_definition;
            }
        }
    }
    return nullptr;
}

const IdentifiableSymbolDef* ParsingContext::get_symbol_def(const std::string& symbol_name) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (const auto* identifier_def = dynamic_cast<const IdentifiableSymbolDef*>(symbol_def.get()))
        {
            if (identifier_def->get_internal_symbol_name() == symbol_name)
            {
                return identifier_def;
            }
        }
    }
    return nullptr;
}

const SymbolFnDefinition* ParsingContext::get_function_def(const std::string& function_name) const
{
    for (const auto& global_scope = this->traverse_to_root();
         const auto& symbol_def : global_scope._symbols)
    {
        if (const auto* fn_def = dynamic_cast<const SymbolFnDefinition*>(symbol_def.get()))
        {
            if (fn_def->get_internal_symbol_name() == function_name)
            {
                return fn_def;
            }
        }
    }
    return nullptr;
}

static size_t levenshtein_distance(const std::string& a, const std::string& b)
{
    const size_t len_a = a.size();
    const size_t len_b = b.size();
    std::vector<size_t> prev(len_b + 1), curr(len_b + 1);

    for (size_t j = 0; j <= len_b; ++j)
    {
        prev[j] = j;
    }

    for (size_t i = 1; i <= len_a; ++i)
    {
        curr[0] = i;
        for (size_t j = 1; j <= len_b; ++j)
        {
            const size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[len_b];
}

ISymbolDef* ParsingContext::fuzzy_find(const std::string& symbol_name) const
{
    ISymbolDef* best_match = nullptr;
    size_t best_distance = std::numeric_limits<size_t>::max();

    // We track the best length difference to break ties between multiple substring matches.
    // (e.g. preferring "fact" over "factor" if input is "factorial", or vice versa depending on preference)
    size_t best_len_diff = std::numeric_limits<size_t>::max();

    auto current = this;
    while (current != nullptr)
    {
        for (const auto& symbol_def : current->_symbols)
        {
            std::string candidate_name;
            // (Your casting logic remains the same)
            if (const auto* field_def = dynamic_cast<const FieldSymbolDef*>(symbol_def.get()))
                candidate_name = field_def->get_internal_symbol_name();
            else if (const auto* fn_def = dynamic_cast<const SymbolFnDefinition*>(symbol_def.get()))
                candidate_name = fn_def->get_internal_symbol_name();
            else if (const auto* id_def = dynamic_cast<const IdentifiableSymbolDef*>(symbol_def.get()))
                candidate_name = id_def->get_internal_symbol_name();
            else
                continue;

            const size_t dist = levenshtein_distance(symbol_name, candidate_name);

            // Calculate absolute length difference
            const size_t len_diff =
                symbol_name.size() > candidate_name.size()
                    ? symbol_name.size() - candidate_name.size()
                    : candidate_name.size() - symbol_name.size();

            // Heuristic: If the edit distance is EXACTLY the length difference,
            // it implies one string is a substring of the other (no internal typos).
            // Example: "factorial_recursive" vs "factorial" -> dist 10, len_diff 10.
            const bool is_substring = (dist == len_diff);

            // If it is a substring, we treat the distance as 0 (or very low)
            // to ensure it passes the threshold.
            if (const size_t effective_dist = is_substring ? 0 : dist;
                effective_dist < best_distance)
            {
                best_distance = effective_dist;
                best_len_diff = len_diff;
                best_match = symbol_def.get();
            }
            // Tie-breaker: If distances are equal (e.g., two substring matches),
            // prefer the one with the smaller length difference (closer match).
            else if (effective_dist == best_distance && len_diff < best_len_diff)
            {
                best_len_diff = len_diff;
                best_match = symbol_def.get();
            }
        }
        current = current->_parent_registry.get();
    }

    // Threshold: standard threshold is 4, but
    // substring matches will have effective_dist of 0, so they always pass.
    if (best_distance <= 4)
        return best_match;

    return nullptr;
}

bool ParsingContext::is_field_defined_in_scope(const std::string& variable_name) const
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

bool ParsingContext::is_field_defined_globally(const std::string& field_name) const
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
    std::unique_ptr<IAstType> type
) const
{
    if (is_field_defined_globally(variable_symbol.internal_name))
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Field '{}' is already defined in global scope", variable_symbol.name),
            *type->get_source(),
            type->get_source_position()
        );
    }

    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());
    global_scope._symbols.push_back(
        std::make_unique<FieldSymbolDef>(
            std::move(variable_symbol),
            std::move(type)
        )
    );
}

void ParsingContext::define_variable(
    Symbol variable_sym,
    std::unique_ptr<IAstType> type
) const
{
    if (this->is_global_scope())
    {
        this->define_variable_globally(std::move(variable_sym),  std::move(type));
        return;
    }
    if (is_field_defined_in_scope(variable_sym.internal_name))
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Field '{}' is already defined in this scope", variable_sym.name),
            *type->get_source(),
            type->get_source_position()
        );
    }

    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());

    global_scope._symbols.push_back(
        std::make_unique<FieldSymbolDef>(
            std::move(variable_sym),
            std::move(type)
        )
    );
}

const FieldSymbolDef* ParsingContext::lookup_variable(const std::string& name) const
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

std::optional<StructSymbolDef*> ParsingContext::get_struct_def(const std::string& name) const
{
    auto current = this;

    while (current != nullptr)
    {
        if (current->get_current_scope_type() != ScopeType::GLOBAL && current->get_current_scope_type() !=
            ScopeType::MODULE)
        {
            current = current->_parent_registry.get();
            continue;
        }

        for (const auto& definition : current->_symbols)
        {
            if (auto* struct_def = dynamic_cast<StructSymbolDef*>(definition.get()))
            {
                // Here we don't check for the internal name, as we don't always know what the data
                // layout is initially (which is used for resolving the actual internal name)
                if (struct_def->get_internal_symbol_name() == name)
                {
                    return struct_def;
                }
            }
        }

        current = current->_parent_registry.get();
    }

    return std::nullopt;
}

std::optional<std::vector<std::pair<std::string, IAstType*>>> ParsingContext::get_struct_fields(
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

    return definition.value()->get_fields();
}

void ParsingContext::define_struct(
    const Symbol& struct_symbol,
    std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> fields
) const
{
    if (const auto existing_def = this->get_struct_def(struct_symbol.internal_name);
        existing_def.has_value())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Struct '{}' is already defined in this scope", struct_symbol.name),
            *fields.begin()->second->get_source(),
            fields.begin()->second->get_source_position()
        );
    }

    auto& root = const_cast<ParsingContext&>(this->traverse_to_root());
    root._symbols.push_back(
        std::make_unique<StructSymbolDef>(
            std::move(struct_symbol),
            std::move(fields)
        )
    );
}

void ParsingContext::define_struct(
    const Symbol& struct_name,
    const Symbol& reference_struct_name
) const
{
    if (this->_current_scope != ScopeType::GLOBAL && this->_current_scope != ScopeType::MODULE)
    {
        throw parsing_error(
            "Reference structs can only be defined in the global or module scope"
        );
    }

    auto& root = const_cast<ParsingContext&>(this->traverse_to_root());

    if (const auto existing_def = this->get_struct_def(struct_name.name);
        existing_def.has_value())
    {
        throw parsing_error(
            std::format("Struct '{}' is already defined in this scope", struct_name.name)
        );
    }

    root._symbols.push_back(
        std::make_unique<StructSymbolDef>(
            struct_name,
            reference_struct_name
        )
    );
}

std::vector<std::pair<std::string, IAstType*>> StructSymbolDef::get_fields() const
{
    std::vector<std::pair<std::string, IAstType*>> copy{};
    copy.reserve(this->_fields.size());

    for (const auto& [name, type] : this->_fields)
    {
        copy.emplace_back(name, type.get());
    }

    return std::move(copy);
}

std::optional<IAstType*> StructSymbolDef::get_struct_member_field_type(
    const std::string& field_name,
    const std::vector<std::pair<std::string, IAstType*>>& fields
)
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
std::optional<IAstType*> StructSymbolDef::get_struct_member_field_type(const std::string& field_name)
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

bool StructSymbolDef::is_reference_struct() const
{
    return this->_reference_struct_sym.has_value();
}

std::optional<int> StructSymbolDef::get_struct_field_member_index(const std::string& member_name) const
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
