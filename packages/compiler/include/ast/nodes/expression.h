#pragma once

#include <utility>

#include "errors.h"
#include "ast/symbols.h"

namespace llvm
{
    class Function;
}

namespace stride::ast
{
    enum class VisibilityModifier;
    enum class TokenType;
    class AstLiteral;
    class AstFunctionParameter;
    class ParsingContext;

    namespace definition
    {
        class IDefinition;
    }

    enum class BinaryOpType
    {
        ADD,      // <..> + <..>
        SUBTRACT, // <..> - <..>
        MULTIPLY, // <..> * <..>
        DIVIDE,   // <..> / <..>
        MODULO,   // <..> % <..>
        POWER     // <..> ** <..>
    };

    enum class LogicalOpType
    {
        AND, // <..> && <..>
        OR   // <..> || <..>
    };

    enum class ComparisonOpType
    {
        EQUALS,               // <..> == <..>
        NOT_EQUAL,            // <..> != <..>
        LESS_THAN,            // <..> < <..>
        LESS_THAN_OR_EQUAL,   // <..> <= <..>
        GREATER_THAN,         // <..> > <..>
        GREATER_THAN_OR_EQUAL // <..> >= <..>
    };

    enum class UnaryOpType
    {
        LOGICAL_NOT,       //  !<..>
        NEGATE,            //  -<..>
        PLUS,              //  +<..>
        COMPLEMENT,        //  ~<..>
        INCREMENT_INFIX,   // --<..>
        INCREMENT_POSTFIX, // <..>++
        DECREMENT_INFIX,   // ++<..>
        DECREMENT_POSTFIX, // <..>--
        ADDRESS_OF,        //  &<..>
        DEREFERENCE,       //  *<..>
    };

    enum class MutativeAssignmentType
    {
        ASSIGN,              // <..> = <..>
        ADD,                 // <..> += <..>
        SUBTRACT,            // <..> -= <..>
        MULTIPLY,            // <..> *= <..>
        DIVIDE,              // <..> /= <..>
        MODULO,              // <..> %= <..>
        BITWISE_OR,          // <..> |= <..>
        BITWISE_AND,         // <..> &= <..>
        BITWISE_XOR,         // <..> ^= <..>
        BITWISE_LEFT_SHIFT,  // <..> <<= <..>
        BITWISE_RIGHT_SHIFT, // <..> >>= <..>
        BITWISE_NOT,         // <..> ~= <..>
    };

    class IAstExpression
        : public IAstNode,
          public IReducible
    {
        std::unique_ptr<IAstType> _type;

        friend class IAstFunction;

    public:
        explicit IAstExpression(
            const SourceFragment& source_position,
            const std::shared_ptr<ParsingContext>& context
        ) :
            IAstNode(source_position, context) {}

        ~IAstExpression() override = default;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        [[nodiscard]]
        IAstType* get_type() const
        {
            if (!this->_type)
            {
                throw parsing_error(
                    ErrorType::COMPILATION_ERROR,
                    "Unable to deduce type for expression",
                    this->get_source_fragment()
                );
            }
            return this->_type.get();
        }

        void set_type(std::unique_ptr<IAstType> type)
        {
            this->_type = std::move(type);
        }

        // Must be implemented by children
        bool is_reducible() override
        {
            return false;
        }

        std::optional<std::unique_ptr<IAstNode>> reduce() override
        {
            return std::nullopt;
        }
    };

    using ExpressionList = std::vector<std::unique_ptr<IAstExpression>>;
    using StructMemberInitializerPair = std::pair<std::string, std::unique_ptr<IAstExpression>>;

    class AstArray
        : public IAstExpression
    {
        ExpressionList _elements;

    public:
        explicit AstArray(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            ExpressionList elements
        ) :
            IAstExpression(source, context),
            _elements(std::move(elements)) {}

        [[nodiscard]]
        const ExpressionList& get_elements() const
        {
            return this->_elements;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        void validate() override;

        void resolve_forward_references(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::string to_string() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstIdentifier
        : public IAstExpression
    {
        Symbol _symbol;

    public:
        explicit AstIdentifier(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol
        ) :
            IAstExpression(symbol.symbol_position, context),
            _symbol(std::move(symbol)) {}

        [[nodiscard]]
        std::optional<const definition::IDefinition*> get_definition() const;

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_symbol.name;
        }

        [[nodiscard]]
        const std::string& get_scoped_name() const
        {
            return this->_symbol.internal_name;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::string to_string() override;

        bool is_reducible() override
        {
            return false;
        }

        std::optional<std::unique_ptr<IAstNode>> reduce() override
        {
            return std::nullopt;
        }

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstArrayMemberAccessor
        : public IAstExpression
    {
        std::unique_ptr<IAstExpression> _array_base;
        std::unique_ptr<IAstExpression> _index_accessor_expr;

    public:
        explicit AstArrayMemberAccessor(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> array_base,
            std::unique_ptr<IAstExpression> index_expr
        ) :
            IAstExpression(source, context),
            _array_base(std::move(array_base)),
            _index_accessor_expr(std::move(index_expr)) {}

        [[nodiscard]]
        IAstExpression* get_array_base() const
        {
            return this->_array_base.get();
        }

        [[nodiscard]]
        IAstExpression* get_index() const
        {
            return this->_index_accessor_expr.get();
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    /// Represents a chained postfix expression: base.member, where base is any expression
    /// and followup is an AstIdentifier (the member name). Multi-step chains like a.b.c
    /// are represented left-recursively: ChainedExpression(ChainedExpression(a, b), c).
    class AstChainedExpression
        : public IAstExpression
    {
        std::unique_ptr<IAstExpression> _base;
        std::unique_ptr<IAstExpression> _followup; // always AstIdentifier at each leaf

    public:
        explicit AstChainedExpression(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> base,
            std::unique_ptr<IAstExpression> followup
        ) :
            IAstExpression(source, context),
            _base(std::move(base)),
            _followup(std::move(followup)) {}

        [[nodiscard]]
        IAstExpression* get_base() const
        {
            return this->_base.get();
        }

        [[nodiscard]]
        IAstExpression* get_followup() const
        {
            return this->_followup.get();
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::string to_string() override;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        std::unique_ptr<IAstNode> clone() override;

        void validate() override;

    private:
        llvm::Value* codegen_global_member_accessor(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) const;
    };

    /// Represents an indirect (expression-based) function call: expr(args).
    /// Used when the callee is not a simple named identifier, e.g. arr[i]() or obj.fn().
    class AstIndirectCall
        : public IAstExpression
    {
        std::unique_ptr<IAstExpression> _callee;
        ExpressionList _args;

    public:
        explicit AstIndirectCall(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> callee,
            ExpressionList args
        ) :
            IAstExpression(source, context),
            _callee(std::move(callee)),
            _args(std::move(args)) {}

        [[nodiscard]]
        IAstExpression* get_callee() const
        {
            return this->_callee.get();
        }

        [[nodiscard]]
        const ExpressionList& get_args() const
        {
            return this->_args;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::string to_string() override;

        bool is_reducible() override
        {
            return false;
        }

        std::optional<std::unique_ptr<IAstNode>> reduce() override
        {
            return std::nullopt;
        }

        std::unique_ptr<IAstNode> clone() override;

        void validate() override;
    };

    class AstFunctionCall
        : public IAstExpression
    {
        ExpressionList _arguments;
        std::unique_ptr<AstIdentifier> _function_name_identifier;
        int _flags;

    public:
        explicit AstFunctionCall(
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstIdentifier> function_name_identifier,
            ExpressionList arguments,
            const int flags = SRFLAG_NONE
        ) :
            IAstExpression(function_name_identifier->get_source_fragment(), context),
            _arguments(std::move(arguments)),
            _function_name_identifier(std::move(function_name_identifier)),
            _flags(flags) {}

        [[nodiscard]]
        const ExpressionList& get_arguments() const
        {
            return this->_arguments;
        }

        [[nodiscard]]
        std::vector<std::unique_ptr<IAstType>> get_argument_types() const;

        [[nodiscard]]
        const std::string& get_function_name() const
        {
            return this->_function_name_identifier->get_name();
        }

        [[nodiscard]]
        const std::string& get_scoped_function_name() const
        {
            return this->_function_name_identifier->get_scoped_name();
        }

        [[nodiscard]]
        AstIdentifier* get_function_name_identifier() const
        {
            return this->_function_name_identifier.get();
        }

        [[nodiscard]]
        bool is_variadic() const
        {
            return (this->_flags & SRFLAG_FN_TYPE_VARIADIC) != 0;
        }

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        std::unique_ptr<IAstNode> clone() override;

        void validate() override;

        [[nodiscard]]
        std::string get_formatted_call() const;

    private:
        [[nodiscard]]
        std::string format_function_name() const;

        static std::string format_suggestion(const definition::IDefinition* suggestion);

        llvm::Value* codegen_anonymous_function_call(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) const;

        [[nodiscard]]
        llvm::Function* resolve_regular_callee(
            llvm::Module* module
        ) const;

        llvm::Value* codegen_regular_function_call(
            llvm::Function* callee,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) const;
    };

    class AstVariableDeclaration
        : public IAstExpression
    {
        std::optional<std::unique_ptr<IAstType>> _annotated_type;
        std::unique_ptr<IAstExpression> _initial_value;
        const VisibilityModifier _visibility;

        const Symbol _symbol;

        const int _flags;

    public:
        explicit AstVariableDeclaration(
            const std::shared_ptr<ParsingContext>& context,
            Symbol symbol,
            std::optional<std::unique_ptr<IAstType>> variable_type,
            std::unique_ptr<IAstExpression> initial_value,
            VisibilityModifier visibility,
            const int flags = SRFLAG_NONE
        ) :
            IAstExpression(symbol.symbol_position, context),
            _annotated_type(std::move(variable_type)),
            _initial_value(std::move(initial_value)),
            _visibility(visibility),
            _symbol(std::move(symbol)),
            _flags(flags) {}

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
        bool has_annotated_type() const
        {
            return this->_annotated_type.has_value();
        }

        [[nodiscard]]
        std::optional<IAstType*> get_annotated_type() const
        {
            if (this->_annotated_type.has_value())
                return this->_annotated_type->get();

            return std::nullopt;
        }

        [[nodiscard]]
        IAstExpression* get_initial_value() const
        {
            return this->_initial_value.get();
        }

        std::string to_string() override;

        void resolve_forward_references(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }

    };

    class IBinaryOp
        : public IAstExpression
    {
        std::unique_ptr<IAstExpression> _lhs;
        std::unique_ptr<IAstExpression> _rhs;

    public:
        friend class AstBinaryArithmeticOp;
        friend class AstLogicalOp;
        friend class AstComparisonOp;

        explicit IBinaryOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> lsh,
            std::unique_ptr<IAstExpression> rsh
        ) :
            IAstExpression(source, context),
            _lhs(std::move(lsh)),
            _rhs(std::move(rsh)) {}

        [[nodiscard]]
        IAstExpression* get_left() const
        {
            return this->_lhs.get();
        }

        [[nodiscard]]
        IAstExpression* get_right() const
        {
            return this->_rhs.get();
        }
    };

    class AstBinaryArithmeticOp
        : public IBinaryOp
    {
        const BinaryOpType _op_type;

    public:
        explicit AstBinaryArithmeticOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> left,
            const BinaryOpType op,
            std::unique_ptr<IAstExpression> right
        ) :
            IBinaryOp(
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
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        std::unique_ptr<IAstNode> clone() override;

        void validate() override;
    };

    class AstLogicalOp
        : public IBinaryOp
    {
        LogicalOpType _op_type;

    public:
        explicit AstLogicalOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> left,
            const LogicalOpType op,
            std::unique_ptr<IAstExpression> right
        ) :
            IBinaryOp(source, context, std::move(left), std::move(right)),
            _op_type(op) {}

        [[nodiscard]]
        LogicalOpType get_op_type() const
        {
            return this->_op_type;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstComparisonOp
        : public IBinaryOp
    {
        ComparisonOpType _op_type;

    public:
        explicit AstComparisonOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> left,
            const ComparisonOpType op,
            std::unique_ptr<IAstExpression> right
        ) :
            IBinaryOp(source, context, std::move(left), std::move(right)),
            _op_type(op) {}

        [[nodiscard]]
        ComparisonOpType get_op_type() const
        {
            return this->_op_type;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstUnaryOp
        : public IAstExpression
    {
        const UnaryOpType _op_type;
        std::unique_ptr<IAstExpression> _operand;

    public:
        explicit AstUnaryOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const UnaryOpType op,
            std::unique_ptr<IAstExpression> operand
        ) :
            IAstExpression(source, context),
            _op_type(op),
            _operand(std::move(operand)) {}

        [[nodiscard]]
        bool is_postfix_operation() const
        {
            return this->_op_type == UnaryOpType::INCREMENT_POSTFIX
                || this->_op_type == UnaryOpType::DECREMENT_POSTFIX;
        }

        [[nodiscard]]
        UnaryOpType get_op_type() const
        {
            return this->_op_type;
        }

        [[nodiscard]]
        IAstExpression& get_operand() const
        {
            return *this->_operand;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        std::string to_string() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstVariableReassignment
        : public IAstExpression
    {
        std::unique_ptr<AstIdentifier> _identifier;
        std::unique_ptr<IAstExpression> _value;
        MutativeAssignmentType _operator;

        std::optional<std::string> _internal_name;

    public:
        explicit AstVariableReassignment(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstIdentifier> identifier,
            const MutativeAssignmentType op,
            std::unique_ptr<IAstExpression> value
        ) :
            IAstExpression(source, context),
            _identifier(std::move(identifier)),
            _value(std::move(value)),
            _operator(op) {}

        [[nodiscard]]
        const std::string& get_variable_name() const
        {
            return this->_identifier->get_name();
        }

        [[nodiscard]]
        IAstExpression* get_value() const
        {
            return this->_value.get();
        }

        [[nodiscard]]
        AstIdentifier* get_identifier() const
        {
            return this->_identifier.get();
        }

        [[nodiscard]]
        MutativeAssignmentType get_operator() const
        {
            return this->_operator;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        void resolve_forward_references(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        std::string to_string() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    class AstObjectInitializer
        : public IAstExpression
    {
        std::string _object_type_name;
        std::vector<StructMemberInitializerPair> _member_initializers;
        GenericTypeList _generic_type_arguments;

        // Instantiated type, if initializer is generic
        std::unique_ptr<AstObjectType> _object_type = nullptr;

    public:
        explicit AstObjectInitializer(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string struct_name,
            std::vector<StructMemberInitializerPair> member_initializers,
            GenericTypeList generic_type_arguments = {}
        ) :
            IAstExpression(source, context),
            _object_type_name(std::move(struct_name)),
            _member_initializers(std::move(member_initializers)),
            _generic_type_arguments(std::move(generic_type_arguments)) {}

        [[nodiscard]]
        const std::vector<StructMemberInitializerPair>& get_initializers() const
        {
            return _member_initializers;
        }

        [[nodiscard]]
        const std::string& get_struct_name() const
        {
            return _object_type_name;
        }

        [[nodiscard]]
        const GenericTypeList& get_generic_type_arguments() const
        {
            return _generic_type_arguments;
        }

        [[nodiscard]]
        bool has_generic_type_arguments() const
        {
            return !this->_generic_type_arguments.empty();
        }

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;

        void resolve_forward_references(llvm::Module* module, llvm::IRBuilderBase* builder) override;

    private:
        std::unique_ptr<AstObjectType> get_instantiated_object_type();
    };

    class AstVariadicArgReference
        : public IAstExpression
    {
    public:
        explicit AstVariadicArgReference(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context) :
            IAstExpression(source, context) {}

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        std::unique_ptr<IAstNode> clone() override;

        static llvm::Value* init_variadic_reference(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        );

        static void end_variadic_reference(
            llvm::Module* module,
            llvm::IRBuilderBase* builder,
            llvm::Value* va_list_ptr
        );
    };

    class AstTupleInitializer
        : public IAstExpression
    {
        ExpressionList _members;

    public:
        explicit AstTupleInitializer(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            ExpressionList members
        ) :
            IAstExpression(source, context),
            _members(std::move(members)) {}

        [[nodiscard]]
        const ExpressionList& get_members() const
        {
            return this->_members;
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        std::unique_ptr<IAstNode> clone() override;

        void validate() override;
    };

    class AstTypeCastOp
        : public IAstExpression
    {
        std::unique_ptr<IAstExpression> _value;
        std::unique_ptr<IAstType> _target_type;

    public:
        explicit AstTypeCastOp(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstExpression> value,
            std::unique_ptr<IAstType> target_type
        ) :
            IAstExpression(source, context),
            _value(std::move(value)),
            _target_type(std::move(target_type)) {}

        [[nodiscard]]
        IAstExpression* get_value() const
        {
            return this->_value.get();
        }

        [[nodiscard]]
        IAstType* get_target_type() const
        {
            return this->_target_type.get();
        }

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        void validate() override;

        std::unique_ptr<IAstNode> clone() override;
    };

    /* # * # * # * # * # * # * # * # * # * # * # * # * # * # * # *
     #                                                           #
     *                    PARSER FUNCTIONS                       *
     #                                                           #
     * # * # * # * # * # * # * # * # * # * # * # * # * # * # * # */

    /// Parses a complete standalone expression from tokens
    std::unique_ptr<IAstExpression> parse_standalone_expression(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses an expression that appears inline, e.g., within a statement or as a sub-expression
    std::unique_ptr<IAstExpression> parse_inline_expression(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    /// Parses a single part of a standalone expression
    std::unique_ptr<IAstExpression> parse_inline_expression_part(
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
    std::unique_ptr<IAstExpression> parse_function_call(
        const std::shared_ptr<ParsingContext>& context,
        AstIdentifier* identifier,
        TokenSet& set);

    /// Parses a variable assignment statement
    std::optional<std::unique_ptr<AstVariableReassignment>>
    parse_variable_reassignment(
        const std::shared_ptr<ParsingContext>& context,
        AstIdentifier* identifier,
        TokenSet& set);

    /// Parses a binary arithmetic operation using precedence climbing
    std::optional<std::unique_ptr<IAstExpression>>
    parse_arithmetic_binary_operation_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::unique_ptr<IAstExpression> lhs,
        int min_precedence
    );

    /// Parses a single chained member access step: consumes `.identifier` and wraps lhs
    std::unique_ptr<AstChainedExpression> parse_chained_member_access(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::unique_ptr<IAstExpression> lhs
    );

    /// Parses a unary operator expression
    std::optional<std::unique_ptr<IAstExpression>> parse_binary_unary_op(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    /// Parses an array initializer expression, e.g., [1, 2, 3]
    std::unique_ptr<AstArray> parse_array_initializer(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    /// Parses an array subscript: consumes `[<index_expression>]` and wraps the base expression
    std::unique_ptr<AstArrayMemberAccessor> parse_array_member_accessor(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::unique_ptr<IAstExpression> array_base
    );

    /// Parses an indirect call: consumes `(<args>)` and wraps the callee expression
    std::unique_ptr<AstIndirectCall> parse_indirect_call(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        std::unique_ptr<IAstExpression> callee
    );

    /// Parses a struct initializer expression into an AstObjectInitializer node
    std::unique_ptr<AstObjectInitializer> parse_object_initializer(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    /// Parses a dot-separated identifier into its individual name segments, e.g., `foo::bar::baz`
    std::unique_ptr<AstIdentifier> parse_segmented_identifier(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        const std::string& error_message
    );

    /// Parses a lambda function literal into an expression node
    std::unique_ptr<IAstExpression> parse_anonymous_fn_expression(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set
    );

    std::optional<std::unique_ptr<IAstExpression>> parse_type_cast_op(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        IAstExpression* lhs
    );

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
    std::optional<UnaryOpType> get_unary_op_type(TokenType type, bool is_infix);

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

    /// Checks whether the next tokens begin a member access: `.identifier`
    bool is_member_accessor(const TokenSet& set);
} // namespace stride::ast
