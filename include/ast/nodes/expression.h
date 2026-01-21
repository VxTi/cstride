#pragma once

#include "ast_node.h"
#include "types.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define SRFLAG_EXPR_ALLOW_VARIABLE_DECLARATION  1
#define SRFLAG_EXPR_INLINE_VARIABLE_DECLARATION 2 // Variables declared after initial one
#define SRFLAG_EXPR_VARIABLE_ASSIGNATION        4

#define SRFLAG_VAR_DECL_EXTERN  (0x1)
#define SRFLAG_VAR_DECL_MUTABLE (0x2)
#define SRFLAG_VAR_DECL_GLOBAL  (0x4)
#define SRFLAG_VAR_DECL_ARRAY   (0x8)

#define SR_PROPERTY_ACCESSOR_SEPARATOR ("@")

#define SR_EXPRESSION_MAX_IDENTIFIER_RESOLUTION (100)

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

    enum class MutativeAssignmentType
    {
        ASSIGN,
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        MODULO,
        BITWISE_OR,
        BITWISE_AND,
        BITWISE_XOR
    };

    /// Base class for all expression AST nodes.

    class AstExpression :
        public IAstNode,
        public ISynthesisable,
        public IReducible
    {
        const std::vector<std::unique_ptr<IAstNode>> _children;

    public:
        explicit AstExpression(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::vector<std::unique_ptr<IAstNode>> children
        ) :
            IAstNode(source, source_offset), _children(std::move(children)) {};

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
        const std::string _name;
        std::string _internal_name;

    public:
        explicit AstIdentifier(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string name,
            std::string internal_name
        ) :
            AstExpression(source, source_offset, {}),
            _name(std::move(name)),
            _internal_name(std::move(internal_name)) {}

        [[nodiscard]]
        const std::string& get_name() const { return this->_name; }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_internal_name.empty() ? this->_name : this->_internal_name;
        }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override { return false; };

        IAstNode* reduce() override { return this; };
    };

    class AstFunctionInvocation :
        public AstExpression
    {
        std::vector<std::unique_ptr<AstExpression>> _arguments;
        const std::string _function_name;
        const std::string _internal_name;

    public:
        explicit AstFunctionInvocation(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string function_name,
            std::string internal_name
        ) :
            AstExpression(source, source_offset, {}),
            _function_name(std::move(function_name)),
            _internal_name(std::move(internal_name)) {}

        explicit AstFunctionInvocation(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string function_name,
            std::string internal_name,
            std::vector<std::unique_ptr<AstExpression>> arguments
        ) :
            AstExpression(source, source_offset, {}),
            _arguments(std::move(arguments)),
            _function_name(std::move(function_name)),
            _internal_name(std::move(internal_name)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstExpression>>& get_arguments() const { return this->_arguments; }

        [[nodiscard]]
        const std::string& get_function_name() const { return this->_function_name; }

        [[nodiscard]]
        const std::string& get_internal_name() const { return this->_internal_name; }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    class AstVariableDeclaration :
        public AstExpression
    {
        const int _flags;
        const std::string _variable_name;
        const std::string _internal_name;
        const std::unique_ptr<IAstInternalFieldType> _variable_type;
        const std::unique_ptr<AstExpression> _initial_value;

    public:
        AstVariableDeclaration(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string variable_name,
            std::unique_ptr<IAstInternalFieldType> variable_type,
            std::unique_ptr<AstExpression> initial_value,
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
        const std::string& get_variable_name() const
        {
            return this->_variable_name;
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_internal_name.empty() ? this->_variable_name : this->_internal_name;
        }

        [[nodiscard]]
        IAstInternalFieldType* get_variable_type() const
        {
            return this->_variable_type.get();
        }

        [[nodiscard]]
        const std::unique_ptr<AstExpression>& get_initial_value() const
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
        AstExpression& get_left() const { return *this->_lsh; }

        [[nodiscard]]
        AstExpression& get_right() const { return *this->_rsh; }
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
        std::unique_ptr<AstExpression> _operand;
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

        bool is_reducible() override;

        IAstNode* reduce() override;

        std::string to_string() override;
    };

    class AstVariableReassignment :
        public AstExpression
    {
        const std::string _variable_name;
        const std::string _internal_name;
        std::unique_ptr<AstExpression> _value;
        MutativeAssignmentType _operator;

    public:
        explicit AstVariableReassignment(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::string variable_name,
            std::string internal_name,
            const MutativeAssignmentType op,
            std::unique_ptr<AstExpression> value
        )
            : AstExpression(source, source_offset, {}),
              _variable_name(std::move(variable_name)),
              _internal_name(std::move(internal_name)),
              _value(std::move(value)),
              _operator(op) {}

        [[nodiscard]]
        const std::string& get_variable_name() const { return _variable_name; }

        [[nodiscard]]
        AstExpression* get_value() const { return this->_value.get(); }

        [[nodiscard]]
        const std::string& get_internal_name() const { return this->_internal_name; }

        [[nodiscard]]
        MutativeAssignmentType get_operator() const { return this->_operator; }

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        bool is_reducible() override;

        IAstNode* reduce() override;

        std::string to_string() override;
    };


    /// Checks if the token set represents a variable declaration
    bool is_variable_declaration(const TokenSet& set);

    /// Returns the precedence value for a binary arithmetic operator
    int binary_operator_precedence(BinaryOpType type);

    /// Parses a complete standalone expression from tokens
    std::unique_ptr<AstExpression> parse_standalone_expression(std::shared_ptr<Scope> scope, TokenSet& set);

    /// Parses an expression with extended flags controlling variable declarations and assignments
    std::unique_ptr<AstExpression> parse_expression_ext(
        int expression_type_flags,
        std::shared_ptr<Scope> scope,
        TokenSet& set
    );

    /// Parses a single part of a standalone expression
    std::unique_ptr<AstExpression> parse_standalone_expression_part(std::shared_ptr<Scope> scope, TokenSet& set);

    /// Parses a variable declaration statement
    std::unique_ptr<AstVariableDeclaration> parse_variable_declaration(
        int expression_type_flags,
        std::shared_ptr<Scope> scope,
        TokenSet& set
    );

    /// Parses a variable assignment statement
    std::optional<std::unique_ptr<AstVariableReassignment>> parse_variable_reassignment(
        std::shared_ptr<Scope> scope,
        TokenSet& set
    );

    /// Parses a binary arithmetic operation using precedence climbing
    std::optional<std::unique_ptr<AstExpression>> parse_arithmetic_binary_op(
        std::shared_ptr<Scope> scope,
        TokenSet& set,
        std::unique_ptr<AstExpression> lhs,
        int min_precedence
    );

    /// Parses a unary operator expression
    std::optional<std::unique_ptr<AstExpression>> parse_binary_unary_op(std::shared_ptr<Scope> scope, TokenSet& set);

    /// Converts a token type to its corresponding logical operator type
    std::optional<LogicalOpType> get_logical_op_type(TokenType type);

    /// Converts a token type to its corresponding comparison operator type
    std::optional<ComparisonOpType> get_comparative_op_type(TokenType type);

    /// Converts a token type to its corresponding binary arithmetic operator type
    std::optional<BinaryOpType> get_binary_op_type(TokenType type);

    /// Converts a token type to its corresponding unary operator type
    std::optional<UnaryOpType> get_unary_op_type(TokenType type);

    /// Whether the next sequence of tokens is a variable/function access by property reference
    /// e.g., <identifier>.<accessor>
    bool is_property_accessor_statement(const TokenSet& set);

    /// Parses a property accessor statement, e.g., <identifier>.<accessor>
    std::string parse_property_accessor_statement(std::shared_ptr<Scope> scope, TokenSet& set);

    /// Will attempt to resolve the provided expression into an IAstInternalFieldType
    std::unique_ptr<IAstInternalFieldType> resolve_expression_internal_type(
        const std::shared_ptr<Scope>& scope, AstExpression* expr);
}
