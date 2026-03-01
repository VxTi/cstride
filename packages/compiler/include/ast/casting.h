#pragma once

#include <type_traits>

namespace stride::ast
{
    class IAstExpression;
    class IAstType;
    class IAstNode;

    template <typename To, typename From>
    [[nodiscard]] decltype(auto) cast_expr(From&& expr)
    {
        static_assert(
            std::is_base_of_v<IAstExpression, std::remove_pointer_t<To>>,
            "Template parameter From must be derived from IAstExpression");
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

    template <typename To, typename From>
    [[nodiscard]] decltype(auto) cast_ast(From&& type)
    {
        static_assert(
            std::is_base_of_v<IAstNode, std::remove_pointer_t<To>>,
            "Template parameter From must be derived from IAstNode");
        return dynamic_cast<To>(type);
    }
} // namespace stride::ast
