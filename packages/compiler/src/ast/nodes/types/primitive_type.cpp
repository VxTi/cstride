#include "ast/casting.h"
#include "ast/nodes/literal_values.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

size_t stride::ast::get_primitive_bit_count(const PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::INT8:
        return 8;
    case PrimitiveType::INT16:
        return 16;
    case PrimitiveType::INT32:
        return 32;
    case PrimitiveType::INT64:
        return 64;
    case PrimitiveType::UINT8:
        return 8;
    case PrimitiveType::UINT16:
        return 16;
    case PrimitiveType::UINT32:
        return 32;
    case PrimitiveType::UINT64:
        return 64;
    case PrimitiveType::FLOAT32:
        return 32;
    case PrimitiveType::FLOAT64:
        return 64;
    case PrimitiveType::BOOL:
        return 1;
    case PrimitiveType::CHAR:
    case PrimitiveType::STRING:
        return 8;
    case PrimitiveType::VOID:
    case PrimitiveType::NIL:
    default:
        return 0;
    }
}

std::string primitive_type_to_str_internal(const PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::INT8:
        return "i8";
    case PrimitiveType::INT16:
        return "i16";
    case PrimitiveType::INT32:
        return "i32";
    case PrimitiveType::INT64:
        return "i64";
    case PrimitiveType::UINT8:
        return "u8";
    case PrimitiveType::UINT16:
        return "u16";
    case PrimitiveType::UINT32:
        return "u32";
    case PrimitiveType::UINT64:
        return "u64";
    case PrimitiveType::FLOAT32:
        return "f32";
    case PrimitiveType::FLOAT64:
        return "f64";
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
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_INT16:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT16,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_INT32:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT32,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_INT64:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::INT64,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT8:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT8,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT16:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT16,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_UINT32:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT32,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_UINT64:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::UINT64,
            context_type_flags);
    }
    break;
    case TokenType::PRIMITIVE_FLOAT32:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::FLOAT32,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_FLOAT64:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::FLOAT64,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_BOOL:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::BOOL,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_CHAR:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::CHAR,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_STRING:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::STRING,
            context_type_flags
        );
    }
    break;
    case TokenType::PRIMITIVE_VOID:
    {
        result = std::make_unique<AstPrimitiveType>(
            reference_token.get_source_fragment(),
            context,
            PrimitiveType::VOID,
            context_type_flags
        );
    }
    break;
    default:
        break;
    }

    if (!result.has_value())
        return std::nullopt;

    set.skip(offset + 1);

    return parse_type_metadata(
        std::move(result.value()),
        set,
        context_type_flags
    );
}

bool AstPrimitiveType::equals(const IAstType& other) const
{
    if (const auto* other_primitive = dynamic_cast<const AstPrimitiveType*>(&other))
    {
        // If either types is optional, and the other is NIL, they're also "equal".
        const auto is_one_optional =
            (this->get_primitive_type() == PrimitiveType::NIL && other_primitive->is_optional()) ||
            (other_primitive->get_primitive_type() == PrimitiveType::NIL && this->is_optional());

        return this->get_primitive_type() == other_primitive->get_primitive_type() ||
            is_one_optional;
    }

    if (const auto* struct_type = dynamic_cast<const AstNamedType*>(&other))
    {
        if (this->get_primitive_type() == PrimitiveType::NIL && struct_type->is_optional())
        {
            return true;
        }

        return struct_type->equals(*this);
    }

    return false;
}

bool AstPrimitiveType::is_assignable_to_impl(IAstType* other)
{
    if (const auto other_primitive = dynamic_cast<AstPrimitiveType*>(other))
    {
        // Check if both sides are integers or both sides are floating-point types
        if ((this->is_integer_ty() && other_primitive->is_integer_ty()) ||
            (this->is_fp() && other_primitive->is_fp()))
        {
            // We can upcast, though downcasting must be done explicitly.
            return this->bit_count() <= other_primitive->bit_count()
                && this->is_signed_int_ty() == other_primitive->is_signed_int_ty();
            // Ensure both are either signed or unsigned
        }
    }

    return false;
}

// Casting can be done both ways - Low -> Hi, and Hi -> Low
bool AstPrimitiveType::is_castable_to_impl(IAstType* other)
{
    if (const auto other_primitive = dynamic_cast<AstPrimitiveType*>(other))
    {
        return (this->is_integer_ty() || this->is_fp()) && (other_primitive->is_integer_ty() || other_primitive->
            is_fp());
    }
    return false;
}

bool AstPrimitiveType::is_integer_ty() const
{
    switch (this->_type)
    {
    case PrimitiveType::INT8:
    case PrimitiveType::INT16:
    case PrimitiveType::INT32:
    case PrimitiveType::INT64:
    case PrimitiveType::UINT8:
    case PrimitiveType::UINT16:
    case PrimitiveType::UINT32:
    case PrimitiveType::UINT64:
    case PrimitiveType::CHAR:
    case PrimitiveType::BOOL: // 1 bit, still an int
        return true;
    default:
        return false;
    }
}

bool AstPrimitiveType::is_signed_int_ty() const
{
    if (!this->is_integer_ty())
        return false;

    switch (this->_type)
    {
    case PrimitiveType::INT8:
    case PrimitiveType::INT16:
    case PrimitiveType::INT32:
    case PrimitiveType::INT64:
        return true;
    default:
        return false;
    }
}

bool AstPrimitiveType::is_fp() const
{
    switch (this->_type)
    {
    case PrimitiveType::FLOAT32:
    case PrimitiveType::FLOAT64:
        return true;
    default:
        return false;
    }
}

std::unique_ptr<IAstNode> AstPrimitiveType::clone()
{
    return std::make_unique<AstPrimitiveType>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_primitive_type(),
        this->get_flags()
    );
}
