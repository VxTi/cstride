#pragma once
#include "ast_node.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstModule : public AstNode
    {
      Symbol _name;

    public:
        llvm::Value* codegen() override;

        std::string to_string() override;

        explicit AstModule(Symbol name) : _name(std::move(name)) {}

        [[nodiscard]] Symbol name() const { return _name; }

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstModule> try_parse(const Scope& scope, TokenSet& tokens);
    };
}
