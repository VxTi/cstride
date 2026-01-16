#include "ast/nodes/primitive_type.h"

#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::string AstPrimitiveType::to_string()
{
    return std::format("PrimitiveType({})", token_type_to_str(this->type()));
}

std::string AstCustomType::to_string()
{
    return std::format("CustomType({})", this->name());
}

std::optional<std::unique_ptr<AstPrimitiveType>> AstPrimitiveType::try_parse(TokenSet& set)
{
    std::optional<std::unique_ptr<AstPrimitiveType>> result = std::nullopt;
    switch (set.peak_next().type)
    {
    case TokenType::PRIMITIVE_INT8:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_INT8, 1));
        }
        break;
    case TokenType::PRIMITIVE_INT16:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_INT16, 2));
        }
        break;
    case TokenType::PRIMITIVE_INT32:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_INT32, 4));
        }
        break;
    case TokenType::PRIMITIVE_INT64:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_INT64, 8));
        }
        break;
    case TokenType::PRIMITIVE_FLOAT32:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_FLOAT32, 4));
        }
        break;
    case TokenType::PRIMITIVE_FLOAT64:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_FLOAT64, 8));
        }
        break;
    case TokenType::PRIMITIVE_BOOL:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_BOOL, 1));
        }
        break;
    case TokenType::PRIMITIVE_CHAR:
        {
            result = std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_CHAR, 1));
        }
        break;
    case TokenType::PRIMITIVE_STRING:
        {
            result =  std::make_unique<AstPrimitiveType>(AstPrimitiveType(TokenType::PRIMITIVE_STRING, 1));
        }
        break;
    default:
        break;
    }

    if (result.has_value())
    {
        set.next();
    }

    return result;
}

std::optional<std::unique_ptr<AstCustomType>> AstCustomType::try_parse(TokenSet& set)
{
    // Custom types are identifiers in type position.
    if (set.peak_next().type != TokenType::IDENTIFIER)
    {
        return std::nullopt;
    }

    const auto name = set.next().lexeme;
    return std::move(std::make_unique<AstCustomType>(AstCustomType(name)));
}

std::unique_ptr<AstType> stride::ast::try_parse_type(TokenSet& tokens)
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
        tokens.except("Expected a type in function parameter declaration");
    }

    return std::move(type_ptr);
}
