#include "ast/nodes/types.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::string stride::ast::primitive_type_to_str(const PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::INT8: return "i8";
    case PrimitiveType::INT16: return "i16";
    case PrimitiveType::INT32: return "i32";
    case PrimitiveType::INT64: return "i64";
    case PrimitiveType::UINT8: return "u8";
    case PrimitiveType::UINT16: return "u16";
    case PrimitiveType::UINT32: return "u32";
    case PrimitiveType::UINT64: return "u64";
    case PrimitiveType::FLOAT32: return "f32";
    case PrimitiveType::FLOAT64: return "f64";
    case PrimitiveType::BOOL: return "bool";
    case PrimitiveType::CHAR: return "char";
    case PrimitiveType::STRING: return "string";
    case PrimitiveType::VOID: return "void";
    default:
        {
            return "unknown";
        }
    }
}

std::string AstPrimitiveFieldType::to_string()
{
    return std::format("{}{}", primitive_type_to_str(this->type()), this->is_pointer() ? "*" : "");
}

std::string AstNamedValueType::to_string()
{
    return std::format("{}{}", this->name(), this->is_pointer() ? "*" : "");
}

std::optional<std::unique_ptr<AstPrimitiveFieldType>> stride::ast::parse_primitive_type_optional(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set,
    int context_type_flags
)
{
    const auto reference_token = set.peak_next();
    const bool is_ptr = set.peak_next_eq(TokenType::STAR);
    const bool is_reference = set.peak_next_eq(TokenType::AMPERSAND);

    int additional_flags = 0;

    if (is_ptr)
    {
        additional_flags |= SRFLAG_TYPE_PTR;
    }
    else if (is_reference)
    {
        additional_flags |= SRFLAG_TYPE_REFERENCE;
    }

    // If it has flags, we'll have to offset the next token peaking by one
    const int offset = additional_flags ? 1 : 0;
    context_type_flags |= additional_flags;

    std::optional<std::unique_ptr<AstPrimitiveFieldType>> result = std::nullopt;

    switch (set.peak(offset).type)
    {
    case TokenType::PRIMITIVE_INT8:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::INT8,
                1,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT16:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::INT16,
                2,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT32:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::INT32,
                4,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT64:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::INT64,
                8,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT8:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::UINT8,
                1,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT16:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::UINT16,
                2,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT32:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::UINT32,
                4,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT64:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::UINT64,
                8,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_FLOAT32:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::FLOAT32,
                4,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_FLOAT64:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::FLOAT64,
                8,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_BOOL:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::BOOL,
                1,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_CHAR:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::CHAR,
                1,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_STRING:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::STRING,
                1,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_VOID:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                scope,
                PrimitiveType::VOID,
                0,
                context_type_flags
            );
        }
        break;
    default:
        break;
    }

    if (result.has_value())
    {
        set.skip(offset + 1);
    }

    return result;
}

std::optional<std::unique_ptr<AstNamedValueType>> stride::ast::parse_named_type_optional(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set,
    int context_type_flags
)
{
    // Custom types are identifiers in type position.
    const auto reference_token = set.peak_next();

    if (set.peak_next_eq(TokenType::STAR))
    {
        set.next();
        context_type_flags |= SRFLAG_TYPE_PTR;
    }
    if (set.peak_next().type != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto name = set.next().lexeme;
    return std::make_unique<AstNamedValueType>(
        set.source(),
        reference_token.offset,
        scope,
        name,
        context_type_flags
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::parse_ast_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set,
    const std::string& error,
    const int context_flags
)
{
    if (auto primitive = parse_primitive_type_optional(scope, set, context_flags);
        primitive.has_value())
    {
        return std::move(primitive.value());
    }

    if (auto named_type = parse_named_type_optional(scope, set, context_flags);
        named_type.has_value())
    {
        return std::move(named_type.value());
    }

    set.throw_error(error);
}

llvm::Type* stride::ast::internal_type_to_llvm_type(
    IAstInternalFieldType* type,
    [[maybe_unused]] llvm::Module* module,
    llvm::LLVMContext& context
)
{
    if (const auto* primitive = dynamic_cast<AstPrimitiveFieldType*>(type))
    {
        switch (primitive->type())
        {
        case PrimitiveType::INT8:
        case PrimitiveType::UINT8:
            return llvm::Type::getInt8Ty(context);
        case PrimitiveType::INT16:
        case PrimitiveType::UINT16:
            return llvm::Type::getInt16Ty(context);
        case PrimitiveType::INT32:
        case PrimitiveType::UINT32:
            return llvm::Type::getInt32Ty(context);
        case PrimitiveType::INT64:
        case PrimitiveType::UINT64:
            return llvm::Type::getInt64Ty(context);
        case PrimitiveType::FLOAT32:
            return llvm::Type::getFloatTy(context);
        case PrimitiveType::FLOAT64:
            return llvm::Type::getDoubleTy(context);
        case PrimitiveType::BOOL:
            return llvm::Type::getInt1Ty(context);
        case PrimitiveType::CHAR:
            return llvm::Type::getInt8Ty(context);
        case PrimitiveType::STRING:
            return llvm::PointerType::get(context, 0);
        case PrimitiveType::VOID:
        default:
            return llvm::Type::getVoidTy(context);
        }
    }

    if (const auto* custom = dynamic_cast<AstNamedValueType*>(type))
    {
        // If it's a pointer, we don't even need to look up the struct name
        // to return the LLVM type, because all pointers are the same.
        // However, usually you want to validate the type exists first.
        if (custom->is_pointer())
        {
            return llvm::PointerType::get(context, 0); // Replaces PointerType::get(struct_type, 0)
        }

        llvm::StructType* struct_type = llvm::StructType::getTypeByName(context, custom->name());
        if (!struct_type)
        {
            throw parsing_error(
                make_ast_error(
                    *custom->source,
                    custom->source_offset,
                    "Custom type '" + custom->name() + "' not found"
                )
            );
        }

        return struct_type;
    }

    return nullptr;
}

std::unique_ptr<IAstInternalFieldType> stride::ast::get_dominant_type(
    const std::shared_ptr<SymbolRegistry>& scope,
    const IAstInternalFieldType* lhs,
    const IAstInternalFieldType* rhs
)
{
    const auto* lhs_primitive = dynamic_cast<const AstPrimitiveFieldType*>(lhs);
    const auto* rhs_primitive = dynamic_cast<const AstPrimitiveFieldType*>(rhs);
    const auto* lhs_named = dynamic_cast<const AstNamedValueType*>(lhs);
    const auto* rhs_named = dynamic_cast<const AstNamedValueType*>(rhs);

    // Error if one is named and the other is primitive
    if ((lhs_named && rhs_primitive) || (lhs_primitive && rhs_named))
    {
        throw parsing_error(
            make_ast_error(
                *lhs->source,
                lhs->source_offset,
                "Cannot mix primitive type with named type"
            )
        );
    }

    // Both must be primitives for dominance calculation
    if (!lhs_primitive || !rhs_primitive)
    {
        throw parsing_error(
            make_ast_error(
                *lhs->source,
                lhs->source_offset,
                "Cannot compute dominant type for non-primitive types"
            )
        );
    }

    const bool are_both_sides_integers = lhs_primitive->is_integer() && rhs_primitive->is_integer();
    const bool are_both_sides_floats = lhs_primitive->is_fp() && rhs_primitive->is_fp();

    // TODO: Handle unsigned / signed properly

    // If both sides are the same, we'll just return the one with the highest byte count
    // E.g., dominant type (fp32, fp64) will yield fp64, (i32, i64) will yield i64
    if (are_both_sides_floats || are_both_sides_integers)
    {
        return lhs_primitive->byte_size() >= rhs_primitive->byte_size()
                   ? lhs_primitive->clone()
                   : rhs_primitive->clone();
    }

    // If LHS is a float, but the RHS is not, we'll have to convert the resulting
    // type into a float with the highest byte size
    if (lhs_primitive->is_fp() && !rhs_primitive->is_fp())
    {
        // If the RHS has a higher byte size, we need to promote the RHS to a floating point type
        // and return the highest byte size
        if (rhs_primitive->byte_size() > lhs_primitive->byte_size())
        {
            return std::make_unique<AstPrimitiveFieldType>(
                lhs_primitive->source,
                lhs_primitive->source_offset,
                scope,
                PrimitiveType::FLOAT64,
                rhs_primitive->byte_size(),
                rhs_primitive->get_flags()
            );
        }

        // Otherwise, just return the LHS as the dominant type (float32 / float64)
        return lhs_primitive->clone();
    }

    throw parsing_error(
        make_ast_error(
            *rhs->source,
            rhs->source_offset,
            "Cannot compute dominant type for incompatible primitive types"
        )
    );
}

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

size_t stride::ast::ast_type_to_internal_id(IAstInternalFieldType* type)
{
    if (const auto primitive = dynamic_cast<AstPrimitiveFieldType*>(type); primitive != nullptr)
    {
        return primitive_type_to_internal_id(primitive->type());
    }

    if (const auto* named = dynamic_cast<const AstNamedValueType*>(type))
    {
        return std::hash<std::string>{}(named->name());
    }

    return 0x00;
}
