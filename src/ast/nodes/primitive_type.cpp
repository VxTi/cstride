#include "ast/nodes/primitive_type.h"

#include "ast/tokens/token_set.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>

using namespace stride::ast::types;

std::string primitive_type_to_str(PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::INT8: return "int8";
    case PrimitiveType::INT16: return "int16";
    case PrimitiveType::INT32: return "int32";
    case PrimitiveType::INT64: return "int64";
    case PrimitiveType::FLOAT32: return "float32";
    case PrimitiveType::FLOAT64: return "float64";
    case PrimitiveType::BOOL: return "bool";
    case PrimitiveType::CHAR: return "char";
    case PrimitiveType::STRING: return "string";
    case PrimitiveType::VOID: return "void";
    default: return "unknown";
    }
}

std::string AstPrimitiveType::to_string()
{
    return std::format("PrimitiveType({}{})", primitive_type_to_str(this->type()), this->is_pointer() ? "*" : "");
}

std::string AstCustomType::to_string()
{
    return std::format("CustomType({}{})", this->name(), this->is_pointer() ? "*" : "");
}

std::optional<std::unique_ptr<AstPrimitiveType>> AstPrimitiveType::try_parse(TokenSet& set)
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

    std::optional<std::unique_ptr<AstPrimitiveType>> result = std::nullopt;
    switch (set.peak(offset).type)
    {
    case TokenType::PRIMITIVE_INT8:
        {
            result = std::make_unique<AstPrimitiveType>(
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
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT16, 2, flags);
        }
        break;
    case TokenType::PRIMITIVE_INT32:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT32, 4, flags);
        }
        break;
    case TokenType::PRIMITIVE_INT64:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::INT64, 8, flags);
        }
        break;
    case TokenType::PRIMITIVE_FLOAT32:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::FLOAT32, 4, flags);
        }
        break;
    case TokenType::PRIMITIVE_FLOAT64:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::FLOAT64, 8, flags);
        }
        break;
    case TokenType::PRIMITIVE_BOOL:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::BOOL, 1, flags);
        }
        break;
    case TokenType::PRIMITIVE_CHAR:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::CHAR, 1, flags);
        }
        break;
    case TokenType::PRIMITIVE_STRING:
        {
            result = std::make_unique<AstPrimitiveType>(
                set.source(),
                reference_token.offset,
                PrimitiveType::STRING, 1, flags);
        }
        break;
    case TokenType::PRIMITIVE_VOID:
        {
            result = std::make_unique<AstPrimitiveType>(
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

std::optional<std::unique_ptr<AstCustomType>> AstCustomType::try_parse(TokenSet& set)
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
    return std::make_unique<AstCustomType>(
        set.source(),
        reference_token.offset,
        name,
        is_ptr
    );
}

std::unique_ptr<AstType> stride::ast::types::parse_primitive_type(TokenSet& tokens)
{
    std::unique_ptr<AstType> type_ptr;

    if (auto primitive = AstPrimitiveType::try_parse(tokens); primitive.has_value())
    {
        type_ptr = std::move(primitive.value());
    }
    else if (auto custom_type = AstCustomType::try_parse(tokens); custom_type.has_value())
    {
        type_ptr = std::move(custom_type.value());
    }
    else
    {
        tokens.throw_error("Expected a type in function parameter declaration");
    }

    return std::move(type_ptr);
}

llvm::Type* stride::ast::types::internal_type_to_llvm_type(
    AstType* type,
    llvm::Module* module,
    llvm::LLVMContext& context
)
{
    if (const auto* primitive = dynamic_cast<AstPrimitiveType*>(type))
    {
        switch (primitive->type())
        {
        case PrimitiveType::INT8:
            return llvm::Type::getInt8Ty(context);
        case PrimitiveType::INT16:
            return llvm::Type::getInt16Ty(context);
        case PrimitiveType::INT32:
            return llvm::Type::getInt32Ty(context);
        case PrimitiveType::INT64:
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

    if (const auto* custom = dynamic_cast<AstCustomType*>(type))
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
