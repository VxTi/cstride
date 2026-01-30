#include "ast/symbol_registry.h"

#include <algorithm>
#include "errors.h"

using namespace stride::ast;

std::string stride::ast::scope_type_to_str(const ScopeType& type)
{
    switch (type)
    {
    case ScopeType::GLOBAL: return "global";
    case ScopeType::FUNCTION: return "function";
    case ScopeType::CLASS: return "class";
    case ScopeType::BLOCK: return "block";
    case ScopeType::MODULE: return "module";
    }
    return "unknown";
}

const SymbolRegistry& SymbolRegistry::traverse_to_root() const
{
    auto current = this;
    while (current->_parent_registry)
    {
        current = current->_parent_registry.get();
    }
    return *current;
}

bool SymbolRegistry::is_function_defined_globally(const std::string& internal_function_name) const
{
    for (const auto& root = this->traverse_to_root();
         const auto& symbol : root._symbols)
    {
        if (const auto* fn_def = dynamic_cast<const SymbolFnDefinition*>(symbol.get()))
        {
            if (fn_def->get_internal_symbol_name() == internal_function_name)
            {
                return true;
            }
        }
    }

    return false;
}

bool SymbolRegistry::is_symbol_type_defined_globally(
    const std::string& symbol_name,
    const SymbolType& type
) const
{
    for (const auto& root = this->traverse_to_root();
         const auto& symbol : root._symbols)
    {
        if (const auto* identifiable = dynamic_cast<const IdentifiableSymbolDef*>(symbol.get()))
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

void SymbolRegistry::define_function(
    std::string internal_function_name,
    std::vector<std::unique_ptr<IAstInternalFieldType>> parameter_types,
    std::unique_ptr<IAstInternalFieldType> return_type
) const
{
    auto& global_scope = const_cast<SymbolRegistry&>(this->traverse_to_root());
    global_scope._symbols.push_back(
        std::make_unique<SymbolFnDefinition>(
            std::move(parameter_types),
            std::move(return_type),
            std::move(internal_function_name)
        )
    );
}


void SymbolRegistry::define_symbol(const std::string& symbol_name, const SymbolType type)
{
    this->_symbols.push_back(std::make_unique<IdentifiableSymbolDef>(
        type,
        symbol_name
    ));
}

const FieldSymbolDef* SymbolRegistry::get_variable_def(const std::string& variable_name) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (const auto* field_definition = dynamic_cast<const FieldSymbolDef*>(symbol_def.get()))
        {
            if (field_definition->get_internal_symbol_name() == variable_name ||
                field_definition->get_variable_name() == variable_name)
            {
                return field_definition;
            }
        }
    }
    return nullptr;
}

const IdentifiableSymbolDef* SymbolRegistry::get_symbol_def(const std::string& symbol_name) const
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


const SymbolFnDefinition* SymbolRegistry::get_function_def(const std::string& function_name) const
{
    const auto& global_scope = this->traverse_to_root();
    for (const auto& symbol_def : global_scope._symbols)
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
        prev[j] = j;

    for (size_t i = 1; i <= len_a; ++i)
    {
        curr[0] = i;
        for (size_t j = 1; j <= len_b; ++j)
        {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[len_b];
}

ISymbolDef* SymbolRegistry::fuzzy_find(const std::string& symbol_name) const
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

            size_t dist = levenshtein_distance(symbol_name, candidate_name);

            // Calculate absolute length difference
            size_t len_diff = (symbol_name.size() > candidate_name.size())
                                  ? symbol_name.size() - candidate_name.size()
                                  : candidate_name.size() - symbol_name.size();

            // Heuristic: If the edit distance is EXACTLY the length difference,
            // it implies one string is a substring of the other (no internal typos).
            // Example: "factorial_recursive" vs "factorial" -> dist 10, len_diff 10.
            const bool is_substring = (dist == len_diff);

            // If it is a substring, we treat the distance as 0 (or very low)
            // to ensure it passes the threshold.
            const size_t effective_dist = is_substring ? 0 : dist;

            // Update Best Match
            if (effective_dist < best_distance)
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
