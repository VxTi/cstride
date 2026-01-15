#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class StringLiteral : public AstNode
    {
    public:
        const std::string value;

        explicit StringLiteral(std::string val) : value(std::move(val))
        {
        }

        ~StringLiteral() override = default;


        llvm::Value* codegen() override;

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstNode>> try_parse(Scope, TokenSet& tokens);
    };

    class IntegerLiteral : public AstNode
    {
    public:
        const int value;

        explicit IntegerLiteral(const int value) : value(value)
        {
        }

        llvm::Value* codegen() override;

        std::string to_string() override;

        static std::optional<std::unique_ptr<AstNode>> try_parse(Scope, TokenSet& tokens);
    };

    static std::optional<std::unique_ptr<AstNode>> try_parse_literal(const Scope& scope, TokenSet& tokens);
}
