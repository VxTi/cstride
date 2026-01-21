#pragma once
#include <string>

#include "types.h"

namespace stride::ast
{
    std::string resolve_internal_function_name(
        const std::vector<IAstInternalFieldType*>& parameter_types,
        const std::string& function_name
    );
}
