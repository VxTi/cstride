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
    case PrimitiveType::FLOAT32: return "f32";
    case PrimitiveType::FLOAT64: return "f64";
    case PrimitiveType::BOOL: return "bool";
    case PrimitiveType::CHAR: return "char";
    case PrimitiveType::STRING: return "string";
    case PrimitiveType::VOID: return "void";
    default: return "unknown";
    }
}

std::string AstPrimitiveFieldType::to_string()
{
    return std::format("Primitive({}{})", primitive_type_to_str(this->type()), this->is_pointer() ? "*" : "");
}

std::string AstNamedValueType::to_string()
{
    return std::format("Custom({}{})", this->name(), this->is_pointer() ? "*" : "");
}

std::optional<std::unique_ptr<AstPrimitiveFieldType>>
AstPrimitiveFieldType::parse_primitive_type_optional(TokenSet& set)
{
    int flags = 0;
    const auto reference_token = set.peak_next();
    const bool is_ptr = set.peak_next_eq(TokenType::STAR);
    const bool is_reference = set.peak_next_eq(TokenType::AMPERSAND);

    if (is_ptr)
    {
        flags |= SRFLAG_TYPE_PTR;
    }
    else if (is_reference)
    {
        flags |= SRFLAG_TYPE_REFERENCE;
    }

    // If it has flags, we'll have to offset the next token peaking by one
    const int offset = flags ? 1 : 0;

    std::optional<std::unique_ptr<AstPrimitiveFieldType>> result = std::nullopt;
    switch (set.peak(offset).type)
    {
    case TokenType::PRIMITIVE_INT8:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT8,
                1,
                flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT16:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT16, 2, flags);
        }
        break;
    case TokenType::PRIMITIVE_INT32:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT32, 4, flags);
        }
        break;
    case TokenType::PRIMITIVE_INT64:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT64, 8, flags);
        }
        break;
    case TokenType::PRIMITIVE_UINT8:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::UINT8,
                1,
                flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT16:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::UINT16, 2, flags);
        }
        break;
    case TokenType::PRIMITIVE_UINT32:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::UINT32, 4, flags);
        }
        break;
    case TokenType::PRIMITIVE_UINT64:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::UINT64, 8, flags);
        }
        break;
    case TokenType::PRIMITIVE_FLOAT32:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::FLOAT32, 4, flags);
        }
        break;
    case TokenType::PRIMITIVE_FLOAT64:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::FLOAT64, 8, flags);
        }
        break;
    case TokenType::PRIMITIVE_BOOL:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::BOOL, 1, flags);
        }
        break;
    case TokenType::PRIMITIVE_CHAR:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::CHAR, 1, flags);
        }
        break;
    case TokenType::PRIMITIVE_STRING:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::STRING, 1, flags);
        }
        break;
    case TokenType::PRIMITIVE_VOID:
        {
            result = std::make_unique<AstPrimitiveFieldType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::VOID, 0, flags);
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

std::optional<std::unique_ptr<AstNamedValueType>> AstNamedValueType::parse_named_type_optional(TokenSet& set)
{
    // Custom types are identifiers in type position.
    bool is_ptr = false;
    const auto reference_token = set.peak_next();

    if (set.peak_next_eq(TokenType::STAR))
    {
        set.next();
        is_ptr = true;
    }
    if (set.peak_next().type != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto name = set.next().lexeme;
    return std::make_unique<AstNamedValueType>(
        set.source(),
        reference_token.offset,
        name,
        is_ptr
    );
}

std::unique_ptr<IAstInternalFieldType> stride::ast::parse_ast_type(TokenSet& set)
{
    std::unique_ptr<IAstInternalFieldType> type_ptr;

    if (auto primitive = AstPrimitiveFieldType::parse_primitive_type_optional(set); primitive.has_value())
    {
        type_ptr = std::move(primitive.value());
    }
    else if (auto named_type = AstNamedValueType::parse_named_type_optional(set); named_type.has_value())
    {
        type_ptr = std::move(named_type.value());
    }
    else
    {
        set.throw_error("Expected a type in function parameter declaration");
    }

    return std::move(type_ptr);
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

std::unique_ptr<IAstInternalFieldType> stride::ast::get_dominant_type(const IAstInternalFieldType* lhs,
                                                                      const IAstInternalFieldType* rhs)
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

    // Check if both are integer types (signed or unsigned)
    const bool lhs_is_int = lhs_primitive->type() >= PrimitiveType::INT8 &&
        lhs_primitive->type() <= PrimitiveType::UINT64;
    const bool rhs_is_int = rhs_primitive->type() >= PrimitiveType::INT8 &&
        rhs_primitive->type() <= PrimitiveType::UINT64;

    // Check if both are float types
    const bool lhs_is_float = lhs_primitive->type() == PrimitiveType::FLOAT32 ||
        lhs_primitive->type() == PrimitiveType::FLOAT64;
    const bool rhs_is_float = rhs_primitive->type() == PrimitiveType::FLOAT32 ||
        rhs_primitive->type() == PrimitiveType::FLOAT64;

    // Both must be same category (both int or both float)
    if (!((lhs_is_int && rhs_is_int) || (lhs_is_float && rhs_is_float)))
    {
        throw parsing_error(
            make_ast_error(
                *lhs->source,
                lhs->source_offset,
                "Cannot compute dominant type for incompatible primitive types"
            )
        );
    }

    // Return type with larger byte size
    if (lhs_primitive->byte_size() >= rhs_primitive->byte_size())
    {
        return std::make_unique<AstPrimitiveFieldType>(
            lhs_primitive->source,
            lhs_primitive->source_offset,
            lhs_primitive->type(),
            lhs_primitive->byte_size(),
            lhs_primitive->get_flags()
        );
    }

    return std::make_unique<AstPrimitiveFieldType>(
        rhs_primitive->source,
        rhs_primitive->source_offset,
        rhs_primitive->type(),
        rhs_primitive->byte_size(),
        rhs_primitive->get_flags()
    );
}

size_t stride::ast::ast_type_to_internal_id(IAstInternalFieldType* type)
{
    if (const auto* primitive = dynamic_cast<AstPrimitiveFieldType*>(type); primitive != nullptr)
    {
        switch (primitive->type())
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

    if (const auto* named = dynamic_cast<const AstNamedValueType*>(type))
    {
        return std::hash<std::string>{}(named->name());
    }

    return 0x00;
}
