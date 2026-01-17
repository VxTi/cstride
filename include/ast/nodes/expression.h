#pragma once

#include "ast_node.h"
#include "primitive_type.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define EXPRESSION_ALLOW_VARIABLE_DECLARATION  1
#define EXPRESSION_INLINE_VARIABLE_DECLARATION 2 // Variables declared after initial one
#define EXPRESSION_VARIABLE_ASSIGNATION        4

    class AstExpression :
        public IAstNode,
        public ISynthesisable,
        public IReducible
    {
        const std::vector<std::unique_ptr<IAstNode>> _children;

    public:
        explicit AstExpression(std::vector<std::unique_ptr<IAstNode>> children) : _children(std::move(children)) {};

        ~AstExpression() override = default;

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstNode>>& children() const { return this->_children; }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override { return false; };

        IAstNode* reduce() override { return this; };
    };

    class AstIdentifier
        : public AstExpression
    {
    public:
        const Symbol name;

        explicit AstIdentifier(Symbol name) :
            AstExpression({}),
            name(std::move(name)) {}

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override { return false; };

        IAstNode* reduce() override { return this; };
    };

    class AstFunctionInvocation :
        public AstExpression
    {
        std::vector<std::unique_ptr<IAstNode>> arguments;

    public:
        const Symbol function_name;

        explicit AstFunctionInvocation(Symbol function_name) :
            AstExpression({}),
            function_name(std::move(function_name)) {}

        explicit AstFunctionInvocation(
            Symbol function_name,
            std::vector<std::unique_ptr<IAstNode>> arguments
        ) :
            AstExpression({}),
            arguments(std::move(arguments)),
            function_name(std::move(function_name)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstNode>>& get_arguments() const { return this->arguments; }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    class AstVariableDeclaration :
        public AstExpression
    {
        const Symbol variable_name;
        const std::unique_ptr<types::AstType> variable_type;
        const std::unique_ptr<IAstNode> initial_value;

    public:
        AstVariableDeclaration(
            Symbol variable_name,
            std::unique_ptr<types::AstType> variable_type,
            std::unique_ptr<IAstNode> initial_value
        ) :
            AstExpression({}),
            variable_name(std::move(variable_name)),
            variable_type(std::move(variable_type)),
            initial_value(std::move(initial_value)) {}

        [[nodiscard]]
        const Symbol& get_variable_name() const
        {
            return this->variable_name;
        }

        [[nodiscard]]
        types::AstType& get_variable_type() const
        {
            return *this->variable_type;
        }

        [[nodiscard]]
        const IAstNode* get_initial_value() const
        {
            return this->initial_value.get();
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    enum class BinaryOpType
    {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE
    };

    enum class LogicalOpType
    {
        AND,
        OR
    };

    enum class ComparisonOpType
    {
        EQUAL,
        NOT_EQUAL,
        LESS_THAN,
        LESS_THAN_OR_EQUAL,
        GREATER_THAN,
        GREATER_THAN_OR_EQUAL
    };

    class AstBinaryOp :
        public AstExpression
    {
        std::unique_ptr<AstExpression> _left;
        BinaryOpType _op_type;
        std::unique_ptr<AstExpression> _right;

    public:
        AstBinaryOp(
            std::unique_ptr<AstExpression> left,
            BinaryOpType op,
            std::unique_ptr<AstExpression> right
        ) : AstExpression({}),
            _left(std::move(left)),
            _op_type(op),
            _right(std::move(right)) {}

        [[nodiscard]]
        BinaryOpType get_op_type() const { return this->_op_type; }

        [[nodiscard]]
        AstExpression& get_left() const { return *this->_left.get(); }

        [[nodiscard]]
        AstExpression& get_right() const { return *this->_right.get(); }


        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    class AstLogicalOp :
        public AstExpression
    {
        std::unique_ptr<AstExpression> _left;
        LogicalOpType _op_type;
        std::unique_ptr<AstExpression> _right;

    public:
        AstLogicalOp(
            std::unique_ptr<AstExpression> left,
            const LogicalOpType op,
            std::unique_ptr<AstExpression> right
        ) : AstExpression({}),
            _left(std::move(left)),
            _op_type(op),
            _right(std::move(right)) {}

        [[nodiscard]]
        LogicalOpType get_op_type() const { return this->_op_type; }

        [[nodiscard]]
        AstExpression& get_left() const { return *this->_left.get(); }

        [[nodiscard]]
        AstExpression& get_right() const { return *this->_right.get(); }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    bool is_variable_declaration(const TokenSet& set);

    std::unique_ptr<AstExpression> parse_expression(const Scope& scope, TokenSet& tokens);

    std::unique_ptr<AstExpression> parse_expression_ext(
        int expression_type_flags,
        const Scope& scope,
        TokenSet& set
    );

    bool can_parse_expression(const TokenSet& tokens);

    int operator_precedence(TokenType type);

    bool is_logical_operator(TokenType type);
}
