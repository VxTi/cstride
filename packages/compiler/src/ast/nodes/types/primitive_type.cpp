#include "ast/casting.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::string primitive_type_to_str_internal(const PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::INT8:
        return "int8";
    case PrimitiveType::INT16:
        return "int16";
    case PrimitiveType::INT32:
        return "int32";
    case PrimitiveType::INT64:
        return "int64";
    case PrimitiveType::UINT8:
        return "uint8";
    case PrimitiveType::UINT16:
        return "uint16";
    case PrimitiveType::UINT32:
        return "uint32";
    case PrimitiveType::UINT64:
        return "uint64";
    case PrimitiveType::FLOAT32:
        return "float32";
    case PrimitiveType::FLOAT64:
        return "float64";
    case PrimitiveType::BOOL:
        return "bool";
    case PrimitiveType::CHAR:
        return "char";
    case PrimitiveType::STRING:
        return "string";
    case PrimitiveType::VOID:
        return "void";
    case PrimitiveType::NIL:
        return "nil";
    case PrimitiveType::UNKNOWN:
    default:
        return "unknown";
    }
}

std::string stride::ast::primitive_type_to_str(const PrimitiveType type, const int flags)
{
    const auto base_str = primitive_type_to_str_internal(type);

    return std::format(
        "{}{}{}",
        (flags & SRFLAG_TYPE_PTR) != 0 ? "*" : "",
        base_str,
        (flags & SRFLAG_TYPE_OPTIONAL) != 0 ? "?" : "");
}

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_primitive_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    int context_type_flags
)
{
    const auto reference_token = set.peek_next();
    const bool is_ptr = set.peek_next_eq(TokenType::STAR);
    const bool is_reference = set.peek_next_eq(TokenType::AMPERSAND);

    if (is_ptr)
    {
        context_type_flags |= SRFLAG_TYPE_PTR;
    }
    else if (is_reference)
    {
        context_type_flags |= SRFLAG_TYPE_REFERENCE;
    }

    // If it has flags, we'll have to offset the next token peeking by one
    const int offset = is_ptr || is_reference ? 1 : 0;

    std::optional<std::unique_ptr<IAstType>> result = std::nullopt;

    switch (set.peek(offset).get_type())
    {
    case TokenType::PRIMITIVE_INT8:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT8,
            /* bit_count = */
            1 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_INT16:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT16,
            /* bit_count = */
            2 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_INT32:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT32,
            /* bit_count = */
            4 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_INT64:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT64,
            /* bit_count = */
            8 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT8:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT8,
            /* bit_count = */
            1 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT16:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT16,
            /* bit_count = */
            2 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT32:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT32,
            /* bit_count = */
            4 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT64:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT64,
            /* bit_count = */
            8 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_FLOAT32:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::FLOAT32,
            /* bit_count = */
            4 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_FLOAT64:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::FLOAT64,
            /* bit_count = */
            8 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_BOOL:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::BOOL,
            /* bit_count = */
            1,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_CHAR:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::CHAR,
            /* bit_count = */
            1 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_STRING:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::STRING,
            /* bit_count = */
            1 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_VOID:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::VOID,
            /* bit_count = */
            1 * BITS_PER_BYTE,
            context_type_flags);
    }
    break;
    default:
        break;
    }

    if (!result.has_value())
        return std::nullopt;

    set.skip(offset + 1);

    return parse_type_metadata(std::move(result.value()),
                               set,
                               context_type_flags);
}

bool AstPrimitiveType::equals(IAstType& other)
{
    if (const auto* other_primitive = cast_type<AstPrimitiveType*>(&other))
    {
        // If either types is optional, and the other is NIL, they're also "equal".
        const auto is_one_optional =
            (this->get_type() == PrimitiveType::NIL && other_primitive->is_optional()) ||
            (other_primitive->get_type() == PrimitiveType::NIL && this->is_optional());

        return this->get_type() == other_primitive->get_type() ||
            is_one_optional;
    }

    if (const auto* struct_type = cast_type<AstNamedType*>(&other))
    {
        return this->get_type() == PrimitiveType::NIL && struct_type->is_optional();
    }

    return false;
}
