#include "errors.h"
#include "ast/casting.h"
#include "ast/symbol_table.h"
#include "ast/definitions/function_definition.h"

#include <algorithm>
#include <format>

using namespace stride::ast;
using namespace stride::ast::definition;

std::optional<FunctionDefinition*> SymbolTable::get_function_definition(
    const std::string& function_name,
    const std::vector<std::unique_ptr<IAstType>>& parameter_types,
    const size_t instantiated_generic_count,
    bool is_variadic
)
{
    const auto& global_symbol_table = this->traverse_to_root();

    for (const auto& symbol_def : global_symbol_table->_symbols)
    {
        if (auto* fn_def = dynamic_cast<FunctionDefinition*>(symbol_def.get()))
        {
            if (fn_def->matches_parameter_signature(this, function_name, parameter_types, instantiated_generic_count))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

std::optional<FunctionDefinition*> SymbolTable::get_generic_function_definition(
    const std::string& function_name,
    const size_t function_param_count,
    const size_t instantiated_generic_count
)
{
    for (const auto& global_scope = this->traverse_to_root();
         const auto& symbol_def : global_scope->_symbols)
    {
        if (auto* fn_def = dynamic_cast<FunctionDefinition*>(symbol_def.get()))
        {
            if (fn_def->matches_generic_signature(function_name, function_param_count, instantiated_generic_count))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

std::optional<FunctionDefinition*> SymbolTable::get_function_definition(
    const std::string& function_name,
    // We might call this function with an anonymous type, hence not having `AstFunctionType`
    IAstType* function_type
)
{
    const auto signature = cast_type<AstFunctionType*>(function_type);

    if (!signature)
        return std::nullopt;

    for (const auto& global_scope = this->traverse_to_root();
         const auto& symbol_def : global_scope->_symbols)
    {
        if (auto* fn_def = dynamic_cast<FunctionDefinition*>(symbol_def.get()))
        {
            if (fn_def->matches_type_signature(this, function_name, signature))
            {
                return fn_def;
            }
        }
    }
    return std::nullopt;
}

bool FunctionDefinition::matches_generic_signature(
    const std::string& name,
    const size_t function_param_count,
    const size_t generic_param_count) const
{
    if (this->get_internal_symbol_name() != name)
        return false;

    if (!this->get_type()->is_generic())
        return false;

    return this->get_type()->get_generic_parameter_names().size() == generic_param_count
        && this->get_type()->get_parameter_types().size() == function_param_count;
}

bool FunctionDefinition::matches_type_signature(
    SymbolTable* symbol_table,
    const std::string& name,
    const AstFunctionType* signature
) const
{
    if (this->get_internal_symbol_name() != name)
        return false;

    if (this->get_type()->is_generic() && !signature->is_generic())
        return false;

    const auto& other_params = signature->get_parameter_types();

    return matches_parameter_signature(
        symbol_table,
        name,
        other_params,
        signature->get_generic_parameter_names().size()
    );
}

std::unique_ptr<IDefinition> FunctionDefinition::clone() const
{
    return std::make_unique<FunctionDefinition>(
        _function_type->clone_as<AstFunctionType>(),
        get_symbol(),
        get_visibility(),
        _flags);
}

bool FunctionDefinition::matches_parameter_signature(
    SymbolTable* symbol_table,
    const std::string& internal_function_name,
    const std::vector<std::unique_ptr<IAstType>>& other_parameter_types,
    const size_t generic_argument_count
) const
{
    if (this->get_internal_symbol_name() != internal_function_name)
        return false;

    const auto& self_params = this->_function_type->get_parameter_types();

    if (this->is_variadic())
    {
        if (other_parameter_types.size() < self_params.size())
            return false;
    }
    else
    {
        // Otherwise, we expect the same number of arguments
        if (other_parameter_types.size() != self_params.size())
            return false;
    }

    if (this->get_type()->is_generic() && generic_argument_count > 0)
        return this->get_type()->get_generic_parameter_names().size() == generic_argument_count;

    for (size_t i = 0; i < self_params.size(); i++)
    {
        // Strict equality check - parameters must match exactly,
        // otherwise named overloading with different signatures wouldn't work.
        if (!self_params[i]->equals(symbol_table, other_parameter_types[i].get()))
        {
            return false;
        }
    }

    return true;
}

void SymbolTable::define_function(const Symbol& function_name, IAstFunction* node)
{
    define_function(
        function_name,
        node->get_type()->clone_as<AstFunctionType>(),
        node->get_visibility(),
        node,
        node->get_flags()
    );
}

void SymbolTable::define_generic_function(const Symbol& function_name, IAstFunction* node)
{
    const auto& global_scope = this->traverse_to_root();

    if (is_generic_function_defined(function_name.internal_name, node->get_generic_parameters().size()))
    {
        throw stride_error(
            ErrorType::SEMANTIC_ERROR,
            std::format("Generic function '{}' with {} generic parameters already defined globally",
                        function_name.name,
                        node->get_generic_parameters().size()),
            function_name.symbol_position
        );
    }

    global_scope->_symbols.push_back(
        std::make_unique<FunctionDefinition>(
            node->get_type()->clone_as<AstFunctionType>(),
            function_name,
            node,
            node->get_visibility(),
            node->get_flags()
        )
    );
}

void SymbolTable::define_function(
    Symbol function_name,
    std::unique_ptr<AstFunctionType> function_type,
    const VisibilityModifier visibility,
    IAstFunction* node,
    const int flags
)
{
    const auto& global_scope = this->traverse_to_root();

    if (this->is_function_defined(function_name.internal_name, function_type.get()))
    {
        throw stride_error(
            ErrorType::SEMANTIC_ERROR,
            node->is_anonymous()
            ? "Anonymous function with same signature already defined globally"
            : std::format("Function '{}' already defined globally", function_name.name),
            function_name.symbol_position
        );
    }

    global_scope->_symbols.push_back(
        std::make_unique<FunctionDefinition>(
            std::move(function_type),
            function_name,
            node,
            visibility,
            flags
        )
    );
}

bool SymbolTable::is_generic_function_defined(const std::string& function_name, const size_t instantiated_generic_count)
{
    return std::ranges::any_of(
        this->traverse_to_root()->_symbols,
        [&](const auto& symbol)
        {
            if (const auto* fn_def = dynamic_cast<const FunctionDefinition*>(symbol.get()))
            {
                return fn_def->get_internal_symbol_name() == function_name &&
                    !fn_def->get_type()->get_generic_parameter_names().empty() &&
                    fn_def->get_type()->get_generic_parameter_names().size() == instantiated_generic_count;
            }
            return false;
        }
    );
}

bool SymbolTable::is_function_defined(
    const std::string& function_name,
    const AstFunctionType* function_type
)
{
    return std::ranges::any_of(
        this->traverse_to_root()->_symbols,
        [&](const auto& symbol)
        {
            if (const auto* fn_def = dynamic_cast<const FunctionDefinition*>(symbol.get()))
            {
                if (fn_def->get_type()->is_generic() && !function_type->get_generic_parameter_names().empty())
                {
                    return fn_def->matches_generic_signature(
                        function_name,
                        function_type->get_parameter_types().size(),
                        function_type->get_generic_parameter_names().size());
                }

                return fn_def->matches_type_signature(this, function_name, function_type);
            }
            return false;
        }
    );
}

bool FunctionDefinition::has_generic_instantiation(SymbolTable* symbol_table, const std::vector<std::unique_ptr<IAstType>>& generic_types) const
{
    for (const auto& [instantiated_generic_types, llvm_function, _node] : this->_generic_overloads)
    {
        bool all_equal = true;
        for (size_t i = 0; i < generic_types.size(); i++)
        {
            if (instantiated_generic_types.size() != generic_types.size())
            {
                continue;
            }

            if (!instantiated_generic_types[i]->equals(symbol_table, generic_types[i].get()))
            {
                all_equal = false;
                break;
            }
        }
        if (all_equal)
        {
            return true;
        }
    }

    return false;
}

void FunctionDefinition::add_generic_instantiation(SymbolTable* symbol_table, GenericTypeList generic_overload_types)
{
    if (has_generic_instantiation(symbol_table, generic_overload_types))
        return; // Already instantiated

    auto instantiation = this->_reference_node->instantiate_generic_function_template(symbol_table, generic_overload_types);

    // All other fields will be populated in later stages
    this->_generic_overloads.emplace_back(
        std::move(generic_overload_types),
        nullptr,
        std::move(instantiation)
    );
}

llvm::Function* FunctionDefinition::get_generic_overload_llvm_function(SymbolTable* symbol_table, const GenericTypeList& generic_types) const
{
    for (const auto& [instantiated_generic_types, llvm_function, _node] : this->_generic_overloads)
    {
        bool all_equal = true;
        for (size_t i = 0; i < generic_types.size(); i++)
        {
            if (instantiated_generic_types.size() != generic_types.size())
            {
                continue;
            }

            if (!instantiated_generic_types[i]->equals(symbol_table, generic_types[i].get()))
            {
                all_equal = false;
                break;
            }
        }
        if (all_equal)
        {
            return llvm_function;
        }
    }

    return nullptr;
}
