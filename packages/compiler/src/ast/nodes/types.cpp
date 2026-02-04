#include "ast/nodes/types.h"
#include "ast/symbol_registry.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "ast/nodes/literal_values.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

bool is_array_notation(const TokenSet& set)
{
    return set.peek_eq(TokenType::LSQUARE_BRACKET, 0) && set.peek_eq(TokenType::RSQUARE_BRACKET, 1);
}

std::string primitive_type_to_str_internal(const PrimitiveType type)
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
    case PrimitiveType::NIL: return "nil";
    case PrimitiveType::UNKNOWN:
    default: return "unknown";
    }
}

std::string stride::ast::primitive_type_to_str(
    const PrimitiveType type, const int flags
)
{
    const auto base_str = primitive_type_to_str_internal(type);

    return std::format(
        "{}{}{}",
        (flags & SRFLAG_TYPE_PTR) != 0 ? "*" : "",
        base_str,
        (flags & SRFLAG_TYPE_OPTIONAL) != 0 ? "?" : ""
    );
}

std::unique_ptr<IAstType> parse_type_metadata(
    std::unique_ptr<IAstType> type,
    TokenSet& set,
    int context_type_flags
)
{
    const auto src_pos = set.peek_next().get_source_position();
    const bool is_array = is_array_notation(set);

    if (is_array) set.skip(2);

    // If the preceding token is a question mark, the type is determined
    // to be optional.
    // An example of this would be `i32?` or `i32[]?`
    if (set.peek_next_eq(TokenType::QUESTION))
    {
        set.skip(1);
        context_type_flags |= SRFLAG_TYPE_OPTIONAL;
    }

    type->set_flags(type->get_flags() | context_type_flags);

    if (is_array)
    {
        return std::make_unique<AstArrayType>(
            type->get_source(),
            stride::SourcePosition(src_pos.offset, src_pos.length + 2),
            type->get_registry(),
            std::move(type),
            0
        );
    }

    return std::move(type);
}

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_primitive_type_optional(
    const std::shared_ptr<SymbolRegistry>& registry,
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
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::INT8,
                /* bit_count = */ 1 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT16:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::INT16,
                /* bit_count = */ 2 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT32:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::INT32,
                /* bit_count = */ 4 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_INT64:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::INT64,
                /* bit_count = */ 8 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT8:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::UINT8,
                /* bit_count = */ 1 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT16:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::UINT16,
                /* bit_count = */ 2 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT32:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::UINT32,
                /* bit_count = */ 4 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_UINT64:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::UINT64,
                /* bit_count = */ 8 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_FLOAT32:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::FLOAT32,
                /* bit_count = */ 4 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_FLOAT64:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::FLOAT64,
                /* bit_count = */ 8 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_BOOL:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::BOOL,
                /* bit_count = */ 1,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_CHAR:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::CHAR,
                /* bit_count = */ 1 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_STRING:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::STRING,
                /* bit_count = */ 1 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    case TokenType::PRIMITIVE_VOID:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.get_source(),
                reference_token.get_source_position(),
                registry,
                PrimitiveType::VOID,
                /* bit_count = */ 1 * BITS_PER_BYTE,
                context_type_flags
            );
        }
        break;
    default:
        break;
    }

    if (!result.has_value()) return std::nullopt;

    set.skip(offset + 1);

    return parse_type_metadata(std::move(result.value()), set, context_type_flags);
}

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_named_type_optional(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set,
    int context_type_flags
)
{
    // Custom types are identifiers in the type position.
    const auto reference_token = set.peek_next();

    if (set.peek_next_eq(TokenType::STAR))
    {
        set.next();
        context_type_flags |= SRFLAG_TYPE_PTR;
    }
    if (set.peek_next().get_type() != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto name = set.next().get_lexeme();

    auto named_type = std::make_unique<AstStructType>(
        set.get_source(),
        reference_token.get_source_position(),
        registry,
        name,
        context_type_flags
    );

    return parse_type_metadata(std::move(named_type), set, context_type_flags);
}

std::unique_ptr<IAstType> stride::ast::parse_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set,
    const std::string& error,
    const int context_flags
)
{
    if (auto primitive = parse_primitive_type_optional(registry, set, context_flags);
        primitive.has_value())
    {
        return std::move(primitive.value());
    }

    if (auto named_type = parse_named_type_optional(registry, set, context_flags);
        named_type.has_value())
    {
        return std::move(named_type.value());
    }

    set.throw_error(error);
}

std::string stride::ast::get_root_reference_struct_name(
    const std::string& name,
    const std::shared_ptr<SymbolRegistry>& registry
)
{
    std::string actual_name = name;
    if (auto struct_def_opt = registry->get_struct_def(actual_name);
        struct_def_opt.has_value())
    {
        auto struct_def = struct_def_opt.value();

        while (struct_def->is_reference_struct())
        {
            actual_name = struct_def->get_reference_struct().value().name;
            struct_def_opt = registry->get_struct_def(actual_name);

            if (!struct_def_opt.has_value())
            {
                break;
            }

            struct_def = struct_def_opt.value();
        }
    }
    return actual_name;
}

llvm::Type* stride::ast::internal_type_to_llvm_type(
    IAstType* type,
    llvm::Module* module
)
{
    const auto registry = type->get_registry();

    // Wrapping T -> Optional<T>
    if (type->is_optional())
    {
        const auto inner = type->clone();
        // Remove the optional flag, so that it doesn't recursively enter this same scope
        inner->set_flags(inner->get_flags() & ~SRFLAG_TYPE_OPTIONAL);
        llvm::Type* inner_type = internal_type_to_llvm_type(inner.get(), module);

        return llvm::StructType::get(
            module->getContext(), {
                llvm::Type::getInt1Ty(module->getContext()), // has_value
                inner_type // value (T)
            });
    }

    if (type->is_pointer())
    {
        return llvm::PointerType::get(module->getContext(), 0);
    }

    if (const auto* ast_array_ty = dynamic_cast<AstArrayType*>(type))
    {
        llvm::Type* element_type = internal_type_to_llvm_type(ast_array_ty->get_element_type(), module);

        if (!element_type)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                "Unable to resolve internal type for array element",
                *ast_array_ty->get_source(),
                ast_array_ty->get_source_position()
            );
        }

        return llvm::ArrayType::get(element_type, ast_array_ty->get_initial_length());
    }

    if (const auto* ast_primitive_ty = dynamic_cast<AstPrimitiveType*>(type))
    {
        switch (ast_primitive_ty->get_type())
        {
        case PrimitiveType::INT8:
        case PrimitiveType::UINT8:
            return llvm::Type::getInt8Ty(module->getContext());
        case PrimitiveType::INT16:
        case PrimitiveType::UINT16:
            return llvm::Type::getInt16Ty(module->getContext());
        case PrimitiveType::INT32:
        case PrimitiveType::UINT32:
            return llvm::Type::getInt32Ty(module->getContext());
        case PrimitiveType::INT64:
        case PrimitiveType::UINT64:
            return llvm::Type::getInt64Ty(module->getContext());
        case PrimitiveType::FLOAT32:
            return llvm::Type::getFloatTy(module->getContext());
        case PrimitiveType::FLOAT64:
            return llvm::Type::getDoubleTy(module->getContext());
        case PrimitiveType::BOOL:
            return llvm::Type::getInt1Ty(module->getContext());
        case PrimitiveType::CHAR:
            return llvm::Type::getInt8Ty(module->getContext());
        case PrimitiveType::STRING:
            return llvm::PointerType::get(module->getContext(), 0);
        case PrimitiveType::VOID:
        default:
            return llvm::Type::getVoidTy(module->getContext());
        }
    }

    if (const auto* ast_struct_ty = dynamic_cast<AstStructType*>(type))
    {
        // If it's a pointer, we don't even need to look up the struct name
        // to return the LLVM type, because all pointers are the same.
        // However, usually you want to validate the type exists first.
        if (ast_struct_ty->is_pointer())
        {
            return llvm::PointerType::get(module->getContext(), 0);
        }

        const std::string actual_name = get_root_reference_struct_name(ast_struct_ty->name(), registry);

        llvm::StructType* struct_ty = llvm::StructType::getTypeByName(module->getContext(), actual_name);
        if (!struct_ty)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Struct type '{}' not found", ast_struct_ty->name()),
                *ast_struct_ty->get_source(),
                ast_struct_ty->get_source_position()
            );
        }

        return struct_ty;
    }

    return nullptr;
}

std::unique_ptr<IAstType> stride::ast::get_dominant_field_type(
    const std::shared_ptr<SymbolRegistry>& registry,
    IAstType* lhs,
    IAstType* rhs
)
{
    const auto* lhs_primitive = dynamic_cast<AstPrimitiveType*>(lhs);
    const auto* rhs_primitive = dynamic_cast<AstPrimitiveType*>(rhs);
    const auto* lhs_named = dynamic_cast<AstStructType*>(lhs);
    const auto* rhs_named = dynamic_cast<AstStructType*>(rhs);

    // Error if one is named and the other is primitive
    if ((lhs_named && rhs_primitive) || (lhs_primitive && rhs_named))
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot mix primitive type with named type",
            *lhs->get_source(),
            lhs->get_source_position()
        );
    }

    // Both must be primitives for dominance calculation
    if (!lhs_primitive || !rhs_primitive)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Cannot compute dominant type for non-primitive types",
            *lhs->get_source(),
            lhs->get_source_position()
        );
    }

    const bool are_both_sides_integers = lhs_primitive->is_integer_ty() && rhs_primitive->is_integer_ty();
    const bool are_both_sides_floats = lhs_primitive->is_fp() && rhs_primitive->is_fp();

    // TODO: Handle unsigned / signed properly

    // If both sides are the same, we'll just return the one with the highest byte count
    // E.g., dominant type (fp32, fp64) will yield fp64, (i32, i64) will yield i64
    if (are_both_sides_floats || are_both_sides_integers)
    {
        return lhs_primitive->bit_count() >= rhs_primitive->bit_count()
                   ? lhs_primitive->clone()
                   : rhs_primitive->clone();
    }

    // If LHS is a float, but the RHS is not, we'll have to convert the resulting
    // type into a float with the highest byte size
    // The same holds true for visa vesa.
    if (
        (lhs_primitive->is_fp() && !rhs_primitive->is_fp()) ||
        (rhs_primitive->is_fp() && !lhs_primitive->is_fp())
    )
    {
        // If the RHS has a higher byte size, we need to promote the RHS to a floating point type
        // and return the highest byte size
        if (rhs_primitive->bit_count() > lhs_primitive->bit_count())
        {
            return std::make_unique<AstPrimitiveType>(
                lhs_primitive->get_source(),
                lhs_primitive->get_source_position(),
                registry,
                PrimitiveType::FLOAT64,
                rhs_primitive->bit_count(),
                rhs_primitive->get_flags()
            );
        }

        // Otherwise, just return the LHS as the dominant type (float32 / float64)
        return lhs_primitive->clone();
    }

    const std::vector references = {
        ErrorSourceReference(
            lhs->to_string(),
            *rhs->get_source(),
            lhs->get_source_position()
        ),
        ErrorSourceReference(
            rhs->get_internal_name(),
            *rhs->get_source(),
            rhs->get_source_position()
        )
    };

    throw parsing_error(
        ErrorType::TYPE_ERROR,
        "Cannot compute dominant type for incompatible primitive types",
        references
    );
}

bool AstStructType::equals(IAstType& other)
{
    if (const auto* other_primitive = dynamic_cast<AstPrimitiveType*>(&other))
    {
        // The other type might be a primitive "NIL" type,
        // then we consider the types equal if this is optional
        return other_primitive->get_type() == PrimitiveType::NIL && this->is_optional();
    }
    if (auto* other_named = dynamic_cast<AstStructType*>(&other))
    {
        return this->get_internal_name() == other_named->get_internal_name();
    }
    return false;
}

bool AstPrimitiveType::equals(IAstType& other)
{
    if (const auto* other_primitive = dynamic_cast<AstPrimitiveType*>(&other))
    {
        // If either types is optional, and the other is NIL, they're also "equal".
        const auto is_one_optional =
            (this->get_type() == PrimitiveType::NIL && other_primitive->is_optional())
            || (other_primitive->get_type() == PrimitiveType::NIL && this->is_optional());

        return this->get_type() == other_primitive->get_type() || is_one_optional;
    }

    if (const auto* struct_type = dynamic_cast<AstStructType*>(&other))
    {
        return this->get_type() == PrimitiveType::NIL && struct_type->is_optional();
    }

    return false;
}

bool AstArrayType::equals(IAstType& other)
{
    if (const auto* other_array = dynamic_cast<AstArrayType*>(&other))
    {
        // Length here doesn't matter; merely the types should be the same.
        return this->_element_type->equals(*other_array->_element_type);
    }
    return false;
}
