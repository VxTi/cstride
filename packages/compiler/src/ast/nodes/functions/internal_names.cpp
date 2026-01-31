#include "ast/internal_names.h"

#include <ranges>
#include <__ranges/views.h>

std::string stride::ast::resolve_internal_function_name(
    const std::vector<IAstType*>& parameter_types,
    const std::string& function_name
)
{
    if (function_name == MAIN_FN_NAME) return function_name;

    std::string finalized_name = function_name;

    int type_hash = 0;
    int parameter_offset = 0;

    // Not perfect, but semi unique
    for (const auto& type : parameter_types)
    {
        type_hash |= static_cast<int>(ast_type_to_internal_id(type));
        type_hash <<= parameter_offset;
        parameter_offset += 2;
    }

    return std::format("{}${:06x}", finalized_name, type_hash);
}

std::string stride::ast::resolve_internal_struct_name(
    const std::vector<std::pair<std::string, std::unique_ptr<IAstType>>>& struct_members,
    const std::string& struct_name
)
{
    std::string finalized_name = struct_name;

    int type_hash = 0;
    int parameter_offset = 0;

    // Not perfect, but semi unique
    for (const auto& type : struct_members | std::views::values)
    {
        type_hash |= static_cast<int>(ast_type_to_internal_id(type.get()));
        type_hash <<= parameter_offset;
        parameter_offset += 2;
    }

    return std::format("{}${:06x}", finalized_name, type_hash);
}
