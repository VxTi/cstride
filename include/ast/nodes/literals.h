#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"
#include "expression.h"

namespace stride::ast
{
    class AstLiteral :
        public AstExpression
    {
    public:
        AstLiteral() : AstExpression({}) {}

        std::string to_string() override = 0;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse(const Scope& scope, TokenSet& tokens);
    };

    class AstStringLiteral : public AstLiteral
    {
        const std::string _value;

    public:
        explicit AstStringLiteral(std::string val) : _value(std::move(val))
        {
        }

        ~AstStringLiteral() override = default;

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);

        [[nodiscard]] const std::string& value() const { return this->_value; }
    };

    class AstIntegerLiteral : public AstLiteral
    {
        const int _value;

    public:
        explicit AstIntegerLiteral(const int value) : _value(value)
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);

        [[nodiscard]] int value() const { return this->_value; }
    };

    class AstFloatLiteral : public AstLiteral
    {
        const double _value;

    public :
        explicit AstFloatLiteral(const double value) : _value(value)
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        static std::optional<std::unique_ptr<AstLiteral>> try_parse_optional(const Scope& scope, TokenSet& tokens);

        [[nodiscard]] double value() const { return this->_value; }
    };
}
