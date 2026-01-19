#pragma once

#include "ast_node.h"
#include "primitive_type.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define SRFLAG_EXPR_ALLOW_VARIABLE_DECLARATION  1
#define SRFLAG_EXPR_INLINE_VARIABLE_DECLARATION 2 // Variables declared after initial one
#define SRFLAG_EXPR_VARIABLE_ASSIGNATION        4

#define SRFLAG_VAR_DECL_EXTERN  0x1
#define SRFLAG_VAR_DECL_MUTABLE 0x2
#define SRFLAG_VAR_DECL_GLOBAL  0x4

    enum class BinaryOpType
    {
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        MODULO,
        POWER
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

    enum class UnaryOpType
    {
        LOGICAL_NOT, //  !<..>
        NEGATE,      //  -<..>
        COMPLEMENT,  //  ~<..>
        INCREMENT,   // ++<..> or <..>++
        DECREMENT,   // --<..> or <..>--
        ADDRESS_OF,  //  &<..>
        DEREFERENCE, //  *<..>
    };

    class AstExpression :
        public IAstNode,
        public ISynthesisable,
        public IReducible
    {
        const std::vector<u_ptr<IAstNode>> _children;

    public:
        explicit AstExpression(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::vector<u_ptr<IAstNode>> children
        ) :
            IAstNode(source, source_offset), _children(std::move(children)) {};

        ~AstExpression() override = default;

        [[nodiscard]]
        const std::vector<u_ptr<IAstNode>>& children() const { return this->_children; }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override { return false; };

        IAstNode* reduce() override { return this; };
    };

    class AstIdentifier
        : public AstExpression
    {
        const Symbol _name;
        std::string _internal_name;

    public:
        explicit AstIdentifier(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol name,
            std::string internal_name = ""
        ) :
            AstExpression(source, source_offset, {}),
            _name(std::move(name)),
            _internal_name(std::move(internal_name)) {}

        [[nodiscard]]
        const Symbol& get_name() const { return this->_name; }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_internal_name.empty() ? this->_name.value : this->_internal_name;
        }

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

        explicit AstFunctionInvocation(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol function_name
        ) :
            AstExpression(source, source_offset, {}),
            function_name(std::move(function_name)) {}

        explicit AstFunctionInvocation(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol function_name,
            std::vector<std::unique_ptr<IAstNode>> arguments
        ) :
            AstExpression(source, source_offset, {}),
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
        const int _flags;
        const Symbol _variable_name;
        const std::string _internal_name;
        const u_ptr<types::AstType> _variable_type;
        const u_ptr<IAstNode> _initial_value;

    public:
        AstVariableDeclaration(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            Symbol variable_name,
            u_ptr<types::AstType> variable_type,
            u_ptr<IAstNode> initial_value,
            const int flags,
            std::string internal_name
        ) :
            AstExpression(source, source_offset, {}),

            _flags(flags),
            _variable_name(std::move(variable_name)),
            _internal_name(std::move(internal_name)),
            _variable_type(std::move(variable_type)),
            _initial_value(std::move(initial_value)) {}

        [[nodiscard]]
        const Symbol& get_variable_name() const
        {
            return this->_variable_name;
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_internal_name.empty() ? this->_variable_name.value : this->_internal_name;
        }

        [[nodiscard]]
        types::AstType* get_variable_type() const
        {
            return this->_variable_type.get();
        }

        [[nodiscard]]
        const u_ptr<IAstNode>& get_initial_value() const
        {
            return this->_initial_value;
        }

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    class AbstractBinaryOp : public AstExpression
    {
        std::unique_ptr<AstExpression> _lsh;
        std::unique_ptr<AstExpression> _rsh;

    public:
        AbstractBinaryOp(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::unique_ptr<AstExpression> lsh,
            std::unique_ptr<AstExpression> rsh
        ) :
            AstExpression(source, source_offset, {}),
            _lsh(std::move(lsh)),
            _rsh(std::move(rsh)) {}

        [[nodiscard]]
        AstExpression& get_left() const { return *this->_lsh.get(); }

        [[nodiscard]]
        AstExpression& get_right() const { return *this->_rsh.get(); }
    };

    class AstBinaryArithmeticOp :
        public AbstractBinaryOp
    {
        const BinaryOpType _op_type;

    public:
        AstBinaryArithmeticOp(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::unique_ptr<AstExpression> left,
            const BinaryOpType op,
            std::unique_ptr<AstExpression> right
        ) : AbstractBinaryOp(source, source_offset, std::move(left), std::move(right)),
            _op_type(op) {}

        [[nodiscard]]
        BinaryOpType get_op_type() const { return this->_op_type; }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    class AstLogicalOp :
        public AbstractBinaryOp
    {
        LogicalOpType _op_type;

    public:
        AstLogicalOp(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::unique_ptr<AstExpression> left,
            const LogicalOpType op,
            std::unique_ptr<AstExpression> right
        ) :
            AbstractBinaryOp(source, source_offset, std::move(left), std::move(right)),
            _op_type(op) {}

        [[nodiscard]]
        LogicalOpType get_op_type() const { return this->_op_type; }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    class AstComparisonOp :
        public AbstractBinaryOp
    {
        ComparisonOpType _op_type;

    public:
        AstComparisonOp(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::unique_ptr<AstExpression> left,
            const ComparisonOpType op,
            std::unique_ptr<AstExpression> right
        ) : AbstractBinaryOp(source, source_offset, std::move(left), std::move(right)),
            _op_type(op) {}

        [[nodiscard]]
        ComparisonOpType get_op_type() const { return this->_op_type; }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    class AstUnaryOp
        : public AstExpression
    {
        const UnaryOpType _op_type;
        u_ptr<AstExpression> _operand;
        const bool _is_lsh;

    public:
        AstUnaryOp(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const UnaryOpType op,
            std::unique_ptr<AstExpression> operand,
            const bool is_lsh = false
        ) :
            AstExpression(source, source_offset, {}),
            _op_type(op),
            _is_lsh(is_lsh),
            _operand(std::move(operand)) {}

        [[nodiscard]]
        bool is_lsh() const { return this->_is_lsh; }

        [[nodiscard]]
        UnaryOpType get_op_type() const { return this->_op_type; }

        [[nodiscard]]
        AstExpression& get_operand() const { return *this->_operand.get(); }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;
    };

    bool is_variable_declaration(const TokenSet& set);

    int binary_operator_precedence(BinaryOpType type);

    std::unique_ptr<AstExpression> parse_standalone_expression(const Scope& scope, TokenSet& tokens);

    std::unique_ptr<AstExpression> parse_expression_ext(
        int expression_type_flags,
        const Scope& scope,
        TokenSet& set
    );
    std::unique_ptr<AstExpression> parse_standalone_expression_part(const Scope& scope, TokenSet& set);

    std::unique_ptr<AstVariableDeclaration> parse_variable_declaration(
        int expression_type_flags,
        const Scope& scope,
        TokenSet& set
    );


    std::optional<std::unique_ptr<AstExpression>> parse_arithmetic_binary_op(const Scope& scope,
                                                                             TokenSet& set,
                                                                             std::unique_ptr<AstExpression> lhs,
                                                                             int min_precedence);

    std::optional<std::unique_ptr<AstExpression>> parse_binary_unary_op(const /**/Scope& scope, TokenSet& set);

    // Operation type utility functions

    std::optional<LogicalOpType> get_logical_op_type(TokenType type);

    std::optional<ComparisonOpType> get_comparative_op_type(TokenType type);

    std::optional<BinaryOpType> get_binary_op_type(TokenType type);

    std::optional<UnaryOpType> get_unary_op_type(TokenType type);
}
