#pragma once
#include <memory>
#include <optional>

#include "ast_node.h"
#include "ast/tokens/token_set.h"
#include "ast/tokens/token.h"

namespace stride::ast
{
    class AstType : public IAstNode
    {
    };

    class AstPrimitiveType : public AstType
    {
        const TokenType _type;
    public:
        size_t byte_size;

        explicit AstPrimitiveType(const TokenType type, const size_t byte_size) :
            _type(type),
            byte_size(byte_size)
        {
        }

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstPrimitiveType>> try_parse(TokenSet& set);

        [[nodiscard]] TokenType type() const { return this->_type; }
    };

    class AstCustomType : public AstType
    {
        std::string _name;
    public:

        std::string to_string() override;

        explicit AstCustomType(std::string name) : _name(std::move(name))
        {
        }

        static std::optional<std::unique_ptr<AstCustomType>> try_parse(TokenSet& set);

        [[nodiscard]] std::string name() const { return this->_name; }
    };

    std::unique_ptr<AstType> try_parse_type(TokenSet& set);
}
