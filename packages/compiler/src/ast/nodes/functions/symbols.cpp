#include "ast/symbols.h"

#include <ranges>

using namespace stride::ast;

size_t primitive_type_to_internal_id(const PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::INT8:
        return 0x01;
    case PrimitiveType::INT16:
        return 0x02;
    case PrimitiveType::INT32:
        return 0x03;
    case PrimitiveType::INT64:
        return 0x04;
    case PrimitiveType::UINT8:
        return 0x05;
    case PrimitiveType::UINT16:
        return 0x06;
    case PrimitiveType::UINT32:
        return 0x07;
    case PrimitiveType::UINT64:
        return 0x08;
    case PrimitiveType::FLOAT32:
        return 0x09;
    case PrimitiveType::FLOAT64:
        return 0x0A;
    case PrimitiveType::BOOL:
        return 0x0B;
    case PrimitiveType::CHAR:
        return 0x0C;
    case PrimitiveType::STRING:
        return 0x0D;
    case PrimitiveType::VOID:
        return 0x0E;
    default:
        return 0x00;
    }
}

size_t ast_type_to_internal_id(IAstType* type)
{
    if (const auto primitive = dynamic_cast<AstPrimitiveType*>(type);
        primitive != nullptr)
    {
        return primitive_type_to_internal_id(primitive->get_type());
    }

    if (const auto* named = dynamic_cast<const AstStructType*>(type);
        named != nullptr)
    {
        return std::hash<std::string>{}(named->name());
    }

    return 0x00;
}

Symbol stride::ast::resolve_internal_function_name(
    const std::vector<IAstType*>& parameter_types,
    const std::string& function_name
)
{
    if (function_name == MAIN_FN_NAME)
    {
        return Symbol(function_name);
    }

    std::string params = "";

    // Not perfect, but semi unique
    for (const auto& type : parameter_types)
    {
        params += type->to_string();
    }

    return Symbol(function_name, std::format("{}${:x}", function_name, std::hash<std::string>{}(params)));
}
