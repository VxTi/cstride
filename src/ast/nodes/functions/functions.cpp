#include "ast/nodes/functions.h"

std::string stride::ast::resolve_internal_function_name(
    const std::vector<std::shared_ptr<IAstInternalFieldType>>& parameter_types,
    const std::string& function_name
)
{
    std::string finalized_name = function_name;

    int type_hash = 0;
    int parameter_offset = 0;

    // Not perfect, but semi unique
    for (const auto &type : parameter_types)
    {
        type_hash |= ast_type_to_internal_id(type.get());
        type_hash <<= parameter_offset;
        parameter_offset += 2;
    }

    return std::format("{}${:06x}", finalized_name, type_hash);
}
