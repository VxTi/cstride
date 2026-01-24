#pragma once
#include <string>

#include "types.h"

namespace stride::ast
{
#define MAIN_FN_NAME ("main")

    std::string resolve_internal_function_name(
        const std::vector<IAstInternalFieldType*>& parameter_types,
        const std::string& function_name
    );
}
