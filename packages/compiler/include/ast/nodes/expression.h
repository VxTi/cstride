#pragma once

#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"
#include "ast_node.h"
#include "types.h"

#include <set>
#include <utility>

namespace stride::ast
{
    class AstLiteral;
    class AstFunctionParameter;

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
        PLUS,        //  +<..>
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

    class AstExpression
        : public IAstNode,
          public ISynthesisable,
          public IReducible
    {
    public:
        explicit AstExpression(
            const SourceFragment& source_position,
            const std::shared_ptr<ParsingContext>& context) :
            IAstNode(source_position, context) {}

        ~AstExpression() override = default;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override
        {
            return false;
        }

        IAstNode* reduce() override
        {
            return this;
        }
    };

    class AstArray
        : public AstExpression
    {
        std::vector<std::unique_ptr<AstExpression>> _elements;

    public:
        explicit AstArray(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<AstExpression>> elements) :
            AstExpression(source, context),
            _elements(std::move(elements)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstExpression>>& get_elements() const
        {
            return this->_elements;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        void validate() override;

        std::string to_string() override;
    };

    class AstIdentifier
        : public AstExpression
    {
        Symbol _symbol;

    public:
        explicit AstIdentifier(const std::shared_ptr<ParsingContext>& context,
                               Symbol symbol) :
            AstExpression(symbol.symbol_position, context),
            _symbol(std::move(symbol)) {}

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_symbol.name;
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_symbol.internal_name;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override
        {
            return false;
        }

        IAstNode* reduce() override
        {
            return this;
        }
    };

    class AstArrayMemberAccessor
        : public AstExpression
    {
        std::unique_ptr<AstIdentifier> _array_identifier;
        std::unique_ptr<AstExpression> _index_accessor_expr;

    public:
        explicit AstArrayMemberAccessor(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstIdentifier> array_identifier,
            std::unique_ptr<AstExpression> index_expr) :
            AstExpression(source, context),
            _array_identifier(std::move(array_identifier)),
            _index_accessor_expr(std::move(index_expr)) {}

        [[nodiscard]]
        AstIdentifier* get_array_identifier() const
        {
            return this->_array_identifier.get();
        }

        [[nodiscard]]
        const AstExpression* get_index() const
        {
            return this->_index_accessor_expr.get();
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override;

        IAstNode* reduce() override;

        void validate() override;
    };

    class AstMemberAccessor
        : public AstExpression
    {
        // When adding function chaining support,
        // these types will have to be changed to AstExpression
        std::unique_ptr<AstIdentifier> _base;
        std::vector<std::unique_ptr<AstIdentifier>> _members;
        std::unique_ptr<IAstType> _base_type;

    public:
        explicit AstMemberAccessor(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstIdentifier> base,
            std::vector<std::unique_ptr<AstIdentifier>> members
        );

        [[nodiscard]]
        AstIdentifier* get_base() const
        {
            return this->_base.get();
        }

        [[nodiscard]]
        std::vector<AstIdentifier*> get_members() const;

        [[nodiscard]]
        AstIdentifier* get_last_member() const
        {
            return this->_members.empty()
                ? nullptr
                : this->_members.back().get();
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        std::string to_string() override;

        bool is_reducible() override;

        IAstNode* reduce() override;

        void validate() override;
    };

    class AstFunctionCall
        : public AstExpression
    {
        std::vector<std::unique_ptr<AstExpression>> _arguments;
        const Symbol _symbol;
        int _flags;

    public:
        explicit AstFunctionCall(
            const std::shared_ptr<ParsingContext>& context,
            Symbol function_call_sym,
            std::vector<std::unique_ptr<AstExpression>> arguments,
            int function_call_flags = SRFLAG_NONE
        ) :
            AstExpression(function_call_sym.symbol_position, context),
            _arguments(std::move(arguments)),
            _symbol(std::move(function_call_sym)),
            _flags(function_call_flags) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstExpression>>& get_arguments() const
        {
            return this->_arguments;
        }

        [[nodiscard]]
        const std::string& get_function_name() const
        {
            return this->_symbol.name;
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_symbol.internal_name;
        }

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        bool is_reducible() override;

        IAstNode* reduce() override;

    private:
        [[nodiscard]]
        std::string format_function_name() const;

        static std::string format_suggestion(
            const definition::IDefinition* suggestion);
    };

    class AstVariableDeclaration
        : public AstExpression
    {
        const std::unique_ptr<IAstType> _variable_type;
        const std::unique_ptr<AstExpression> _initial_value;
        const VisibilityModifier _visibility;

        const Symbol _symbol;

    public:
        explicit AstVariableDeclaration(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::unique_ptr<IAstType> variable_type,
            std::unique_ptr<AstExpression> initial_value,
            VisibilityModifier visibility
        ) :
            AstExpression(symbol.symbol_position, context),
            _variable_type(std::move(variable_type)),
            _initial_value(std::move(initial_value)),
            _visibility(visibility),
            _symbol(std::move(symbol)) {}

        [[nodiscard]]
        const std::string& get_variable_name() const
        {
            return this->_symbol.name;
        }

        [[nodiscard]]
        const VisibilityModifier& get_visibility() const
        {
            return this->_visibility;
        }

        [[nodiscard]]
        Symbol get_symbol() const
        {
            return this->_symbol;
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_symbol.internal_name;
        }

        [[nodiscard]]
        IAstType* get_variable_type() const
        {
            return this->_variable_type.get();
        }

        [[nodiscard]]
        const std::unique_ptr<AstExpression>& get_initial_value() const
        {
            return this->_initial_value;
        }

        std::string to_string() override;

        void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        bool is_reducible() override
        {
            return false; /* TODO: Implement*/
        }

        IAstNode* reduce() override
        {
            return this; /* TODO: Implement*/
        }

        void validate() override;
    };

    class AbstractBinaryOp
        : public AstExpression
    {
        std::unique_ptr<AstExpression> _lsh;
        std::unique_ptr<AstExpression> _rsh;

    public:
        explicit AbstractBinaryOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> lsh,
            std::unique_ptr<AstExpression> rsh
        ) :
            AstExpression(source, context),
            _lsh(std::move(lsh)),
            _rsh(std::move(rsh)) {}

        [[nodiscard]]
        AstExpression* get_left() const
        {
            return this->_lsh.get();
        }

        [[nodiscard]]
        AstExpression* get_right() const
        {
            return this->_rsh.get();
        }
    };

    class AstBinaryArithmeticOp
        : public AbstractBinaryOp
    {
        const BinaryOpType _op_type;

    public:
        explicit AstBinaryArithmeticOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> left,
            const BinaryOpType op,
            std::unique_ptr<AstExpression> right
        ) :
            AbstractBinaryOp(
                source,
                context,
                std::move(left),
                std::move(right)
            ),
            _op_type(op) {}

        [[nodiscard]]
        BinaryOpType get_op_type() const
        {
            return this->_op_type;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        bool is_reducible() override;

        IAstNode* reduce() override;
    };

    class AstLogicalOp
        : public AbstractBinaryOp
    {
        LogicalOpType _op_type;

    public:
        explicit AstLogicalOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> left,
            const LogicalOpType op,
            std::unique_ptr<AstExpression> right
        ) :
            AbstractBinaryOp(
                source,
                context,
                std::move(left),
                std::move(right)
            ),
            _op_type(op) {}

        [[nodiscard]]
        LogicalOpType get_op_type() const
        {
            return this->_op_type;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    class AstComparisonOp
        : public AbstractBinaryOp
    {
        ComparisonOpType _op_type;

    public:
        explicit AstComparisonOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> left,
            const ComparisonOpType op,
            std::unique_ptr<AstExpression> right
        ) :
            AbstractBinaryOp(
                source,
                context,
                std::move(left),
                std::move(right)
            ),
            _op_type(op) {}

        [[nodiscard]]
        ComparisonOpType get_op_type() const
        {
            return this->_op_type;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        void validate() override;
    };

    class AstUnaryOp
        : public AstExpression
    {
        const UnaryOpType _op_type;
        std::unique_ptr<AstExpression> _operand;
        const bool _is_lsh;

    public:
        explicit AstUnaryOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const UnaryOpType op,
            std::unique_ptr<AstExpression> operand,
            const bool is_lsh = false
        ) :
            AstExpression(source, context),
            _op_type(op),
            _operand(std::move(operand)),
            _is_lsh(is_lsh) {}

        [[nodiscard]]
        bool is_lsh() const
        {
            return this->_is_lsh;
        }

        [[nodiscard]]
        UnaryOpType get_op_type() const
        {
            return this->_op_type;
        }

        [[nodiscard]]
        AstExpression& get_operand() const
        {
            return *this->_operand;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder
            ) override;

        bool is_reducible() override;

        IAstNode* reduce() override;

        std::string to_string() override;

        void validate() override;
    };

    class AstVariableReassignment
        : public AstExpression
    {
        const std::string _variable_name;
        const std::string _internal_name;
        std::unique_ptr<AstExpression> _value;
        MutativeAssignmentType _operator;

    public:
        explicit AstVariableReassignment(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string variable_name,
            std::string internal_name,
            const MutativeAssignmentType op,
            std::unique_ptr<AstExpression> value
        ) :
            AstExpression(source, context),
            _variable_name(std::move(variable_name)),
            _internal_name(std::move(internal_name)),
            _value(std::move(value)),
            _operator(op) {}

        [[nodiscard]]
        const std::string& get_variable_name() const
        {
            return _variable_name;
        }

        [[nodiscard]]
        AstExpression* get_value() const
        {
            return this->_value.get();
        }

        [[nodiscard]]
        const std::string& get_internal_name() const
        {
            return this->_internal_name;
        }

        [[nodiscard]]
        MutativeAssignmentType get_operator() const
        {
            return this->_operator;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        bool is_reducible() override;

        IAstNode* reduce() override;

        std::string to_string() override;

        void validate() override;
    };

    class AstStructInitializer
        : public AstExpression
    {
        std::string _struct_name;
        std::vector<std::pair<std::string, std::unique_ptr<AstExpression>>>
        _initializers;

    public:
        explicit AstStructInitializer(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string struct_name,
            std::vector<std::pair<std::string, std::unique_ptr<AstExpression>>>
            initializers
        ) :
            AstExpression(source, context),
            _struct_name(std::move(struct_name)),
            _initializers(std::move(initializers)) {}

        [[nodiscard]]
        const std::vector<
            std::pair<std::string, std::unique_ptr<AstExpression>>>&
        get_initializers() const
        {
            return _initializers;
        }

        [[nodiscard]]
        const std::string& get_struct_name() const
        {
            return _struct_name;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        void validate() override;
    };

    class AstVariadicArgReference
        : public AstExpression
    {
    public:
        explicit AstVariadicArgReference(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context) :
            AstExpression(source, context) {}

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        void validate() override;
    };

    /* # * # * # * # * # * # * # * # * # * # * # * # * # * # * # *
     #                                                           #
     *                    PARSER FUNCTIONS                       *
     #                                                           #
     * # * # * # * # * # * # * # * # * # * # * # * # * # * # * # */

    /// Parses a complete standalone expression from tokens
    std::unique_ptr<AstExpression> parse_standalone_expression(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses an expression that appears inline, e.g., within a statement or as a sub-expression
    std::unique_ptr<AstExpression> parse_inline_expression(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses a single part of a standalone expression
    std::unique_ptr<AstExpression> parse_inline_expression_part(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses a variable declaration statement
    std::unique_ptr<AstVariableDeclaration> parse_variable_declaration(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        VisibilityModifier modifier);

    /// Parses a variable declaration that appears inline within a larger expression context
    std::unique_ptr<AstVariableDeclaration> parse_variable_declaration_inline(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        VisibilityModifier modifier);

    /// Parses a function invocation into an AstFunctionCall expression node
    std::unique_ptr<AstExpression> parse_function_call(
        const std::shared_ptr<ParsingContext>& context,
        const SymbolNameSegments& function_name_segments,
        TokenSet& set);

    /// Parses a variable assignment statement
    std::optional<std::unique_ptr<AstVariableReassignment>>
    parse_variable_reassignment(
        const std::shared_ptr<ParsingContext>& context,
        const std::string& variable_name,
        TokenSet& set);

    /// Parses a binary arithmetic operation using precedence climbing
    std::optional<std::unique_ptr<AstExpression>>
    parse_arithmetic_binary_operation_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::unique_ptr<AstExpression> lhs,
        int min_precedence);

    /// This parses both function call chaining, and struct member access
    std::unique_ptr<AstExpression> parse_chained_member_access(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        const std::unique_ptr<AstExpression>& lhs);

    /// Parses a unary operator expression
    std::optional<std::unique_ptr<AstExpression>> parse_binary_unary_op(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses an array initializer expression, e.g., [1, 2, 3]
    std::unique_ptr<AstArray> parse_array_initializer(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses an array member accessor expression, e.g., <array_identifier>[<index_expression>]
    std::unique_ptr<AstExpression> parse_array_member_accessor(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::unique_ptr<AstIdentifier> array_identifier);

    /// Parses a struct initializer expression into an AstStructInitializer node
    std::unique_ptr<AstStructInitializer> parse_struct_initializer(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses a dot-separated identifier into its individual name segments, e.g., `foo::bar::baz`
    SymbolNameSegments parse_segmented_identifier(TokenSet& set);

    /// Parses a lambda function literal into an expression node
    std::unique_ptr<AstExpression> parse_lambda_fn_expression(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /* # * # * # * # * # * # * # * # * # * # * # * # * # * # * # *
     #                                                           #
     *                    GETTER FUNCTIONS                       *
     #                                                           #
     * # * # * # * # * # * # * # * # * # * # * # * # * # * # * # */

    /// Returns the precedence value for a binary arithmetic operator
    int get_binary_operator_precedence(BinaryOpType type);

    /// Converts a token type to its corresponding logical operator type
    std::optional<LogicalOpType> get_logical_op_type(TokenType type);

    /// Converts a token type to its corresponding comparison operator type
    std::optional<ComparisonOpType> get_comparative_op_type(TokenType type);

    /// Converts a token type to its corresponding binary arithmetic operator type
    std::optional<BinaryOpType> get_binary_op_type(TokenType type);

    /// Converts a token type to its corresponding unary operator type
    std::optional<UnaryOpType> get_unary_op_type(TokenType type);

    /* # * # * # * # * # * # * # * # * # * # * # * # * # * # * # *
     #                                                           #
     *                    DEDUCTION FUNCTIONS                    *
     #                                                           #
     * # * # * # * # * # * # * # * # * # * # * # * # * # * # * # */

    /// Checks if the token set represents a variable declaration
    bool is_variable_declaration(const TokenSet& set);

    /// Checks if the token set represents an array initializer
    bool is_array_initializer(const TokenSet& set);

    /// Checks if the token set represents a struct initializer
    /// This is the case if an expression starts with `{ <member> }`
    bool is_struct_initializer(const TokenSet& set);

    /// Checks whether the subsequent tokens might be member accessors
    /// e.g., <identifier>.<accessor>
    bool is_member_accessor(AstExpression* lhs, const TokenSet& set);

    /// Checks if the token set represents a lambda function expression
    bool is_lambda_fn_expression(const TokenSet& set);

    /* # * # * # * # * # * # * # * # * # * # * # * # * # * # * # *
     #                                                           #
     *           EXPRESSION TYPE INFERENCE FUNCTIONS             *
     #                                                           #
     * # * # * # * # * # * # * # * # * # * # * # * # * # * # * # */

    /// Will attempt to resolve the provided expression into an IAstInternalFieldType
    std::unique_ptr<IAstType> infer_expression_type(
        const std::shared_ptr<ParsingContext>& context,
        AstExpression* expr);

    /// Infers the element type of an array expression
    std::unique_ptr<IAstType> infer_array_member_type(
        const std::shared_ptr<ParsingContext>& context,
        const AstArray* array);

    /// Infers the result type of a unary operation
    std::unique_ptr<IAstType> infer_unary_op_type(
        const std::shared_ptr<ParsingContext>& context,
        const AstUnaryOp* operation);

    /// Infers the result type of a binary arithmetic operation
    std::unique_ptr<IAstType> infer_binary_arithmetic_op_type(
        const std::shared_ptr<ParsingContext>& context,
        const AstBinaryArithmeticOp* operation);

    /// Infers the type of a literal expression
    std::unique_ptr<IAstType> infer_expression_literal_type(
        const std::shared_ptr<ParsingContext>& context,
        AstLiteral* literal);

    /// Infers the return type of a function call expression
    std::unique_ptr<IAstType> infer_function_call_return_type(
        const std::shared_ptr<ParsingContext>& context,
        const AstFunctionCall* fn_call);

    /// Infers the type produced by a struct initializer expression
    std::unique_ptr<IAstType> infer_struct_initializer_type(
        const std::shared_ptr<ParsingContext>& context,
        const AstStructInitializer* initializer);

    /// Infers the type of the field accessed via a member accessor expression
    std::unique_ptr<IAstType> infer_member_accessor_type(
        const std::shared_ptr<ParsingContext>& context,
        const AstMemberAccessor* expr);
} // namespace stride::ast
