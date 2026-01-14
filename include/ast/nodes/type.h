#pragma once
#include <memory>
#include <optional>

#include "ast/tokens/token_set.h"
#include "ast/tokens/token.h"

namespace stride::ast
{
    class AstType
    {
    public:
        virtual ~AstType() = default;
    };

    class AstPrimitiveType : public AstType
    {
    public:
        const TokenType type;
        size_t byte_size;

        explicit AstPrimitiveType(const TokenType type, const size_t byte_size) :
            type(type),
            byte_size(byte_size)
        {
        }

        static std::optional<std::unique_ptr<AstPrimitiveType>> try_parse(const std::unique_ptr<TokenSet>& set);
    };

    class AstCustomType : public AstType
    {
    public:
        std::string name;

        explicit AstCustomType(std::string name) : name(std::move(name))
        {
        }

        static std::optional<std::unique_ptr<AstCustomType>> try_parse(const std::unique_ptr<TokenSet>& set);
    };

    std::unique_ptr<AstType> try_parse_type(const std::unique_ptr<TokenSet>& set);
}
