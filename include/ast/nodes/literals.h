#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstLiteral : public AstNode
    {
    public:
        std::string to_string() override = 0;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse(const Scope& scope, TokenSet& tokens);
    };

    class AstStringLiteral : public AstLiteral
    {
    public:
        const std::string value;

        explicit AstStringLiteral(std::string val) : value(std::move(val))
        {
        }

        ~AstStringLiteral() override = default;


        llvm::Value* codegen() override;

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };

    class AstIntegerLiteral : public AstLiteral
    {
    public:
        const int value;

        explicit AstIntegerLiteral(const int value) : value(value)
        {
        }

        llvm::Value* codegen() override;

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };

    class AstFloatLiteral : public AstLiteral
    {
        const float _value;

    public :
        explicit AstFloatLiteral(const float value) : _value(value)
        {
        }

        [[nodiscard]] float value() const { return this->_value; }

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);
    };
}
