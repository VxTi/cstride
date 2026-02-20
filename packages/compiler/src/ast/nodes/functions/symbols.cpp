#include "ast/symbols.h"

#include "ast/parsing_context.h"
#include "formatting.h"

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
    if (const auto primitive = dynamic_cast<AstPrimitiveType*>(type); primitive
        != nullptr)
    {
        return primitive_type_to_internal_id(primitive->get_type());
    }

    if (const auto* named = dynamic_cast<const AstNamedType*>(type); named !=
        nullptr)
    {
        return std::hash<std::string>{}(named->name());
    }

    return 0x00;
}

/**
 * Function names will be internalized by their context name and parameter types.
 * This allows functions to be overloaded by their parameter types, as well as preventing name
 * clashes between different contexts.
 */
Symbol stride::ast::resolve_internal_function_name(
    const std::shared_ptr<ParsingContext>& context,
    const SourceLocation& position,
    const SymbolNameSegments& function_name_segments,
    const std::vector<IAstType*>& parameter_types)
{
    if (function_name_segments.size() == 1 && function_name_segments.at(0) ==
        MAIN_FN_NAME)
    {
        return Symbol(position, MAIN_FN_NAME);
    }

    std::string params;

    // Not perfect, but semi unique
    for (const auto& type : parameter_types)
    {
        params += type->to_string();
    }

    const auto function_name = resolve_internal_name(function_name_segments);

    return Symbol(
        position,
        context->get_name(),
        function_name,
        std::format("{}${:x}",
                    function_name,
                    std::hash<std::string>{}(params)));
}


Symbol stride::ast::resolve_internal_name(
    const std::string& context_name,
    const SourceLocation& position,
    const SymbolNameSegments& segments)
{
    return Symbol(position, context_name, resolve_internal_name(segments));
}

std::string stride::ast::resolve_internal_name(
    const SymbolNameSegments& segments)
{
    return join(segments, DELIMITER);
}
