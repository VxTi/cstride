#pragma once
#include <type_traits>

namespace stride::ast
{
    class AstExpression;
    class IAstType;

    template <typename To, typename From>
    [[nodiscard]] decltype(auto) cast_expr(From&& expr)
    {
        static_assert(
            std::is_base_of_v<AstExpression, std::remove_pointer_t<To>>,
            "Template parameter From must be derived from AstExpression");
        return dynamic_cast<To>(expr);
    }

    template <typename To, typename From>
    [[nodiscard]] decltype(auto) cast_type(From&& type)
    {
        static_assert(
            std::is_base_of_v<IAstType, std::remove_pointer_t<To>>,
            "Template parameter From must be derived from IAstType");
        return dynamic_cast<To>(type);
    }
} // namespace stride::ast
