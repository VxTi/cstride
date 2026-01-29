#pragma once
#include <string>

#include "nodes/types.h"

namespace stride::ast
{
#define MAIN_FN_NAME ("main")

    std::string resolve_internal_function_name(
        const std::vector<IAstInternalFieldType*>& parameter_types,
        const std::string& function_name
    );

    std::string resolve_internal_struct_name(
        const std::vector<std::pair<std::string, std::unique_ptr<IAstInternalFieldType>>>& struct_members,
        const std::string& struct_name
    );
}
