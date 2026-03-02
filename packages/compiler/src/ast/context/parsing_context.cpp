#include "ast/parsing_context.h"

#include "errors.h"
#include "ast/symbols.h"

#include <algorithm>
#include <ranges>

using namespace stride::ast;
using namespace stride::ast::definition;

std::string stride::ast::scope_type_to_str(const ContextType& scope_type)
{
    switch (scope_type)
    {
    case ContextType::GLOBAL:
        return "global";
    case ContextType::FUNCTION:
        return "function";
    case ContextType::CLASS:
        return "class";
    case ContextType::MODULE:
        return "module";
    case ContextType::CONTROL_FLOW:
        return "control_flow";
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

bool ParsingContext::is_function_defined_globally(
    const std::string& internal_function_name) const
{
    return std::ranges::any_of(
        this->traverse_to_root()._symbols,
        [&](const auto& symbol)
        {
            if (const auto* fn_def = dynamic_cast<const FunctionDefinition*>(symbol.
                get()))
            {
                if (fn_def->get_internal_symbol_name() ==
                    internal_function_name)
                {
                    return true;
                }
            }
            return false;
        });
}

void ParsingContext::define_function(
    Symbol function_name,
    std::unique_ptr<AstFunctionType> function_type,
    const int flags
) const
{
    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());

    if (this->is_function_defined_globally(function_name.internal_name))
    {
        throw std::runtime_error(
            std::format("Function '{}' already defined globally", function_name.name)
        );
    }

    global_scope._symbols.push_back(
        std::make_unique<FunctionDefinition>(std::move(function_type), function_name, flags)
    );
}

void ParsingContext::define_symbol(const Symbol& symbol_name, const SymbolType type)
{
    this->_symbols.push_back(
        std::make_unique<IdentifiableSymbolDef>(type, symbol_name)
    );
}

const FieldDefinition* ParsingContext::get_variable_def(
    const std::string& variable_name,
    const bool use_raw_name) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (const auto* field_definition = dynamic_cast<const FieldDefinition*>(
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

const IdentifiableSymbolDef* ParsingContext::get_symbol_def(
    const std::string& symbol_name) const
{
    for (const auto& symbol_def : this->_symbols)
    {
        if (const auto* identifier_def =
            dynamic_cast<const IdentifiableSymbolDef*>(symbol_def.get()))
        {
            if (identifier_def->get_internal_symbol_name() == symbol_name)
            {
                return identifier_def;
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
            curr[j] = std::min({ prev[j] + 1, curr[j - 1] + 1,
                                 prev[j - 1] + cost });
        }
        std::swap(prev, curr);
    }
    return prev[len_b];
}

IDefinition* ParsingContext::fuzzy_find(const std::string& symbol_name) const
{
    IDefinition* best_match = nullptr;
    size_t best_distance = std::numeric_limits<size_t>::max();

    // We track the best length difference to break ties between multiple substring matches.
    // (e.g. preferring "fact" over "factor" if input is "factorial", or vice versa depending on
    // preference)
    size_t best_len_diff = std::numeric_limits<size_t>::max();

    auto current = this;
    while (current != nullptr)
    {
        for (const auto& symbol_def : current->_symbols)
        {
            std::string candidate_name;
            // (Your casting logic remains the same)
            if (const auto* field_def = dynamic_cast<const FieldDefinition*>(symbol_def
               .get()))
                candidate_name = field_def->get_internal_symbol_name();
            else if (const auto* fn_def = dynamic_cast<const FunctionDefinition*>(
                symbol_def.get()))
                candidate_name = fn_def->get_internal_symbol_name();
            else
                continue;

            const size_t dist = levenshtein_distance(
                symbol_name,
                candidate_name);

            // Calculate absolute length difference
            const size_t len_diff = symbol_name.size() > candidate_name.size()
                ? symbol_name.size() - candidate_name.size()
                : candidate_name.size() - symbol_name.size();

            // Heuristic: If the edit distance is EXACTLY the length difference,
            // it implies one string is a substring of the other (no internal typos).
            // Example: "factorial_recursive" vs "factorial" -> dist 10, len_diff 10.
            const bool is_substring = dist == len_diff;

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
            else if (effective_dist == best_distance && len_diff <
                best_len_diff)
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

bool ParsingContext::is_field_defined_in_scope(
    const std::string& variable_name) const
{
    return std::ranges::any_of(
        this->_symbols,
        [&](const auto& symbol_def)
        {
            if (const auto* var_def = dynamic_cast<const FieldDefinition*>(symbol_def.
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
    std::unique_ptr<IAstType> type
) const
{
    if (is_field_defined_globally(variable_symbol.internal_name))
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is already defined in global scope", variable_symbol.name),
            type->get_source_fragment()
        );
    }

    auto& global_scope = const_cast<ParsingContext&>(this->traverse_to_root());
    global_scope._symbols.push_back(
        std::make_unique<FieldDefinition>(
            std::move(variable_symbol),
            std::move(type)
        )
    );
}

void ParsingContext::define_variable(
    Symbol variable_sym,
    std::unique_ptr<IAstType> type
)
{
    if (this->is_global_scope())
    {
        this->define_variable_globally(std::move(variable_sym), std::move(type));
        return;
    }

    if (is_field_defined_in_scope(variable_sym.internal_name))
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Variable '{}' is already defined in this scope", variable_sym.name),
            type->get_source_fragment());
    }

    this->_symbols.push_back(std::make_unique<FieldDefinition>(std::move(variable_sym), std::move(type)));
}

IDefinition* ParsingContext::lookup_symbol(const std::string& symbol_name) const
{
    auto current = this;
    while (current != nullptr)
    {
        for (const auto& definition : current->_symbols)
        {
            if (definition->get_symbol().name == symbol_name)
            {
                return definition.get();
            }
        }
        current = current->_parent_registry.get();
    }
    return nullptr;
}

const FieldDefinition* ParsingContext::lookup_variable(
    const std::string& name,
    const bool use_raw_name
)
const
{
    auto current = this;
    while (current != nullptr)
    {
        if (const auto def = current->get_variable_def(name, use_raw_name))
        {
            return def;
        }
        current = current->_parent_registry.get();
    }
    return nullptr;
}
