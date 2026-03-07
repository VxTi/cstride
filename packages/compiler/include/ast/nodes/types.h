#pragma once

#include "ast_node.h"
#include "formatting.h"
#include "ast/flags.h"

#include <format>
#include <memory>
#include <optional>

namespace llvm
{
    class FunctionType;
    class Type;
}

namespace stride::ast
{
    class TokenSet;

    enum class PrimitiveType
    {
        INT8,
        INT16,
        INT32,
        INT64,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        FLOAT32,
        FLOAT64,
        BOOL,
        CHAR,
        STRING,
        VOID,
        // If the resulting type is NIL, and the context allows for optional types,
        // we can safely ignore the type comparison.
        NIL,
    };

    std::string primitive_type_to_str(PrimitiveType type, int flags = SRFLAG_NONE);

    size_t get_primitive_bit_count(PrimitiveType type);

    class IAstType
        : public IAstNode
    {
        int _flags;

    public:
        explicit IAstType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const int flags
        ) :
            IAstNode(source, context),
            _flags(flags) {}

        ~IAstType() override = default;

        std::unique_ptr<IAstType> clone_ty()
        {
            return this->clone_as<IAstType>();
        }

        [[nodiscard]]
        bool is_pointer() const
        {
            return this->_flags & SRFLAG_TYPE_PTR;
        }

        [[nodiscard]]
        bool is_reference() const
        {
            return this->_flags & SRFLAG_TYPE_REFERENCE;
        }

        [[nodiscard]]
        bool is_mutable() const
        {
            return this->_flags & SRFLAG_TYPE_MUTABLE;
        }

        [[nodiscard]]
        bool is_optional() const
        {
            return this->_flags & SRFLAG_TYPE_OPTIONAL;
        }

        [[nodiscard]]
        bool is_global() const
        {
            return this->_flags & SRFLAG_TYPE_GLOBAL;
        }

        [[nodiscard]]
        bool is_variadic() const
        {
            return this->_flags & SRFLAG_TYPE_VARIADIC;
        }

        [[nodiscard]]
        bool is_function() const
        {
            return this->_flags & SRFLAG_TYPE_FUNCTION;
        }

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }

        void set_flags(const int flags)
        {
            this->_flags = flags;
        }

        [[nodiscard]]
        virtual bool is_void_ty() const
        {
            return false;
        }

        [[nodiscard]]
        virtual std::string get_type_name() = 0;

        [[nodiscard]]
        virtual bool equals(const IAstType& other) const = 0;

        [[nodiscard]]
        bool equals(const std::unique_ptr<IAstType>& other) const
        {
            return this->equals(*other);
        }

        [[nodiscard]]
        bool equals(const IAstType* other) const
        {
            return this->equals(*other);
        }

        /// Checks whether other type is assignable to this one.
        /// Lower bit-count primitives are assignable to higher ones, e.g.,
        /// one can assign a 32-bit int to a int64 type, but not visa versa.
        bool is_assignable_to(IAstType* other);

        virtual bool is_castable_to(IAstType* other);

        [[nodiscard]]
        virtual bool is_primitive() const
        {
            return false;
        }

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

    private:
        virtual bool is_assignable_to_impl(IAstType* other)
        {
            return false;
        }

        virtual bool is_castable_to_impl(IAstType* other)
        {
            return false;
        }
    };

    /// Types like int, float, char, etc.
    class AstPrimitiveType
        : public IAstType
    {
        PrimitiveType _type;

    public:
        explicit AstPrimitiveType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            const PrimitiveType type,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, context, flags),
            _type(type) {}

        ~AstPrimitiveType() override = default;

        [[nodiscard]]
        PrimitiveType get_primitive_type() const
        {
            return this->_type;
        }

        [[nodiscard]]
        bool is_integer_ty() const;

        [[nodiscard]]
        bool is_signed_int_ty() const;

        [[nodiscard]]
        bool is_fp() const;

        [[nodiscard]]
        size_t bit_count() const
        {
            return get_primitive_bit_count(this->_type);
        }

        [[nodiscard]]
        std::unique_ptr<IAstNode> clone() override;

        std::string get_type_name() override
        {
            return primitive_type_to_str(this->get_primitive_type(), this->get_flags());
        }

        std::string to_string() override
        {
            return this->get_type_name();
        }

        [[nodiscard]]
        bool equals(const IAstType& other) const override;

        [[nodiscard]]
        bool is_primitive() const override
        {
            return true;
        }

        [[nodiscard]]
        bool is_void_ty() const override
        {
            return this->_type == PrimitiveType::VOID;
        }

        bool is_castable_to(IAstType* other) override
        {
            return IAstType::is_castable_to(other);
        }

    private:
        bool is_assignable_to_impl(IAstType* other) override;

        bool is_castable_to_impl(IAstType* other) override;
    };

    /// References to other types
    class AstNamedType
        : public IAstType
    {
        std::string _name;

    public:
        explicit AstNamedType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::string name,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, context, flags),
            _name(std::move(name)) {}

        [[nodiscard]]
        std::string get_name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        std::unique_ptr<IAstNode> clone() override
        {
            return std::make_unique<AstNamedType>(
                this->get_source_fragment(),
                this->get_context(),
                this->_name,
                this->get_flags()
            );
        }

        std::string get_type_name() override
        {
            return this->_name;
        }

        std::string to_string() override
        {
            return std::format(
                "{}{}{}",
                this->is_pointer() ? "*" : "",
                this->get_name(),
                this->is_optional() ? "?" : "");
        }

        [[nodiscard]]
        bool equals(const IAstType& other) const override;

        bool is_castable_to(IAstType* other) override
        {
            return IAstType::is_castable_to(other);
        }

        [[nodiscard]]
        std::optional<std::unique_ptr<IAstType>> get_reference_type() const;

        /// Returns the super base type of the reference, e.g., if we have:
        /// type RootType = int32;
        /// type MidType = RootType;
        /// type LeafType = MidType;
        /// Then, calling `get_base_reference_type` on `LeafType` will return `int32`.
        [[nodiscard]] std::optional<std::unique_ptr<IAstType>> get_base_reference_type() const;

    private:
        bool is_assignable_to_impl(IAstType* other) override;

        bool is_castable_to_impl(IAstType* other) override;
    };

    class AstFunctionType
        : public IAstType
    {
        std::vector<std::unique_ptr<IAstType>> _parameters;
        std::unique_ptr<IAstType> _return_type;

    public:
        explicit AstFunctionType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<IAstType>> parameters,
            std::unique_ptr<IAstType> return_type,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, context, flags | SRFLAG_TYPE_FUNCTION | SRFLAG_TYPE_PTR),
            _parameters(std::move(parameters)),
            _return_type(std::move(return_type)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstType>>& get_parameter_types() const
        {
            return _parameters;
        }

        [[nodiscard]]
        const std::unique_ptr<IAstType>& get_return_type() const
        {
            return _return_type;
        }

        [[nodiscard]]
        llvm::FunctionType* get_llvm_type(llvm::Module* module) const;

        [[nodiscard]]
        std::unique_ptr<IAstNode> clone() override;

        std::string get_type_name() override;

        std::string to_string() override
        {
            return get_type_name();
        }

        [[nodiscard]]
        bool equals(const IAstType& other) const override;

        bool is_castable_to(IAstType* other) override
        {
            return IAstType::is_castable_to(other);
        }

    private:
        bool is_assignable_to_impl(IAstType* other) override
        {
            return false;
        }

        bool is_castable_to_impl(IAstType* other) override;
    };

    class AstArrayType
        : public IAstType
    {
        std::unique_ptr<IAstType> _element_type;
        size_t _initial_length;

    public:
        explicit AstArrayType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstType> element_type,
            const size_t initial_length,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(
                source,
                context,
                (element_type ? element_type->get_flags() : 0)
                | flags
                | SRFLAG_TYPE_PTR
            ),
            // Arrays are always ptrs
            _element_type(std::move(element_type)),
            _initial_length(initial_length) {}

        [[nodiscard]]
        std::unique_ptr<IAstNode> clone() override;

        [[nodiscard]]
        IAstType* get_element_type() const
        {
            return this->_element_type.get();
        }

        [[nodiscard]]
        size_t get_initial_length() const
        {
            return this->_initial_length;
        }

        std::string to_string() override;

        [[nodiscard]]
        std::string get_type_name() override
        {
            return std::format("[{}]", this->_element_type->get_type_name());
        }

        [[nodiscard]]
        bool equals(const IAstType& other) const override;

        bool is_castable_to(IAstType* other) override
        {
            return IAstType::is_castable_to(other);
        }

    private:
        bool is_assignable_to_impl(IAstType* other) override;

        bool is_castable_to_impl(IAstType* other) override;
    };

    class AstStructType
        : public IAstType
    {
        std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> _members;

    public:
        explicit AstStructType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> members,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, context, flags),
            _members(std::move(members)) {}

        [[nodiscard]]
        std::vector<std::pair<std::string, std::unique_ptr<IAstType>>> get_members() const;

        [[nodiscard]]
        std::optional<IAstType*> get_member_field_type(const std::string& field_name) const;

        [[nodiscard]]
        std::optional<int> get_member_field_index(const std::string& field_name) const;

        [[nodiscard]]
        std::string get_type_name() override
        {
            return "struct";
        }

        std::string to_string() override
        {
            return this->get_type_name();
        }

        [[nodiscard]]
        bool equals(const IAstType& other) const override;

        bool is_castable_to(IAstType* other) override
        {
            return IAstType::is_castable_to(other);
        }

        [[nodiscard]]
        std::unique_ptr<IAstNode> clone() override;

        [[nodiscard]]
        std::string get_internalized_name() const;

    private:
        bool is_assignable_to_impl(IAstType* other) override
        {
            return false;
        }

        bool is_castable_to_impl(IAstType* other) override
        {
            return false;
        }

        // Struct registration is done in the `resolve_forward_references` pass
        // so that one can reference structs before they're semantically defined.
        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

        void resolve_forward_references(
            ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;
    };

    class AstTupleType
        : public IAstType
    {
        std::vector<std::unique_ptr<IAstType>> _members;

    public:
        explicit AstTupleType(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<IAstType>> members,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, context, flags),
            _members(std::move(members)) {}

        [[nodiscard]]
        std::unique_ptr<IAstNode> clone() override;

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstType>>& get_members() const
        {
            return _members;
        }

        std::string get_type_name() override
        {
            return "tuple";
        }

        std::string to_string() override;

        [[nodiscard]]
        bool equals(const IAstType& other) const override;

        bool is_castable_to(IAstType* other) override
        {
            return IAstType::is_castable_to(other);
        }

        llvm::Value* codegen(llvm::Module* module, llvm::IRBuilderBase* builder) override;

        void resolve_forward_references(
            ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;

    private:
        bool is_assignable_to_impl(IAstType* other) override
        {
            return false;
        }

        bool is_castable_to_impl(IAstType* other) override
        {
            return false;
        }
    };

    std::unique_ptr<IAstType> parse_type(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        const std::string& error,
        int type_flags = SRFLAG_NONE);

    llvm::Type* type_to_llvm_type(IAstType* type, llvm::Module* module, size_t recursion_guard = 0);

    std::unique_ptr<IAstType> get_dominant_field_type(IAstType* lhs, IAstType* rhs);

    std::unique_ptr<IAstType> parse_type_metadata(
        std::unique_ptr<IAstType> base_type,
        TokenSet& set,
        int context_type_flags
    );

    std::optional<std::unique_ptr<IAstType>> parse_primitive_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags);

    std::optional<std::unique_ptr<IAstType>> parse_named_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags);

    std::optional<std::unique_ptr<IAstType>> parse_function_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags);

    std::optional<std::unique_ptr<IAstType>> parse_struct_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags);

    std::optional<std::unique_ptr<IAstType>> parse_tuple_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags);

    /// Will extract the struct type from a named type if possible, otherwise,
    /// will attempt to cast the type directly into a struct type.
    /// This function will never return nullptrs.
    std::optional<AstStructType*> get_struct_type_from_type(IAstType* type);
} // namespace stride::ast
