#pragma once

#include <memory>
#include <optional>
#include <__ranges/join_view.h>

#include "ast_node.h"
#include "formatting.h"
#include "ast/flags.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
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
        // Reserved type for empty arrays;
        // It's impossible to deduce the type from an empty array,
        // as it would otherwise be done by inferring the type of its members.
        // Therefore, we'll leave it as a special case.
        UNKNOWN
    };

    std::string primitive_type_to_str(PrimitiveType type, int flags = SRFLAG_NONE);

    class IAstType :
        public IAstNode
    {
        int _flags;

    public:
        IAstType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            const int flags
        )
            : IAstNode(source, source_position, context),
              _flags(flags) {}

        ~IAstType() override = default;

        [[nodiscard]]
        virtual std::unique_ptr<IAstType> clone() const = 0;

        [[nodiscard]]
        bool is_pointer() const { return this->_flags & SRFLAG_TYPE_PTR; }

        [[nodiscard]]
        bool is_reference() const { return this->_flags & SRFLAG_TYPE_REFERENCE; }

        [[nodiscard]]
        bool is_mutable() const { return this->_flags & SRFLAG_TYPE_MUTABLE; }

        [[nodiscard]]
        bool is_optional() const { return this->_flags & SRFLAG_TYPE_OPTIONAL; }

        [[nodiscard]]
        bool is_global() const { return this->_flags & SRFLAG_TYPE_GLOBAL; }

        [[nodiscard]]
        bool is_variadic() const { return this->_flags & SRFLAG_TYPE_VARIADIC; }

        [[nodiscard]]
        int get_flags() const { return this->_flags; }

        void set_flags(const int flags)
        {
            this->_flags = flags;
        }

        [[nodiscard]]
        virtual std::string get_internal_name() = 0;

        virtual bool equals(IAstType& other) = 0;

        [[nodiscard]]
        virtual bool is_primitive() const { return false; }
    };

    class AstPrimitiveType
        : public IAstType
    {
        PrimitiveType _type;
        size_t _bit_count;

    public:
        explicit AstPrimitiveType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            const PrimitiveType type,
            const size_t bit_count,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, source_position, context, flags),
            _type(type),
            _bit_count(bit_count) {}

        ~AstPrimitiveType() override = default;

        [[nodiscard]]
        PrimitiveType get_type() const { return this->_type; }

        [[nodiscard]]
        bool is_integer_ty() const
        {
            switch (this->_type)
            {
            case PrimitiveType::INT8:
            case PrimitiveType::INT16:
            case PrimitiveType::INT32:
            case PrimitiveType::INT64:
            case PrimitiveType::UINT8:
            case PrimitiveType::UINT16:
            case PrimitiveType::UINT32:
            case PrimitiveType::UINT64:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]]
        bool is_fp() const
        {
            switch (this->_type)
            {
            case PrimitiveType::FLOAT32:
            case PrimitiveType::FLOAT64:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]]
        size_t bit_count() const { return this->_bit_count; }

        [[nodiscard]]
        std::unique_ptr<IAstType> clone() const override
        {
            return std::make_unique<AstPrimitiveType>(*this);
        }

        std::string get_internal_name() override
        {
            return primitive_type_to_str(this->get_type(), this->get_flags());
        }

        std::string to_string() override
        {
            return this->get_internal_name();
        }

        bool equals(IAstType& other) override;

        [[nodiscard]]
        bool is_primitive() const override { return true; }
    };

    class AstNamedType
        : public IAstType
    {
        std::string _name;

    public:
        explicit AstNamedType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            std::string name,
            const int flags = SRFLAG_NONE
        ) :
            IAstType(source, source_position, context, flags),
            _name(std::move(name)) {}

        [[nodiscard]]
        std::string name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        std::unique_ptr<IAstType> clone() const override
        {
            return std::make_unique<AstNamedType>(*this);
        }

        std::string get_internal_name() override { return this->_name; }

        std::string to_string() override
        {
            return std::format(
                "{}{}{}",
                this->is_pointer() ? "*" : "",
                this->name(),
                this->is_optional() ? "?" : ""
            );
        }

        bool equals(IAstType& other) override;
    };

    class AstArrayType
        : public IAstType
    {
        std::unique_ptr<IAstType> _element_type;
        size_t _initial_length;

    public:
        explicit AstArrayType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<IAstType> element_type,
            const size_t initial_length
        ) : IAstType(
                source,
                source_position,
                context,
                // Arrays are always ptrs
                (element_type ? element_type->get_flags() : 0) | SRFLAG_TYPE_PTR
            ),
            _element_type(std::move(element_type)),
            _initial_length(initial_length) {}

        [[nodiscard]]
        std::unique_ptr<IAstType> clone() const override
        {
            auto element_clone = this->_element_type
                                     ? this->_element_type->clone()
                                     : nullptr;

            return std::make_unique<AstArrayType>(
                this->get_source(),
                this->get_source_position(),
                this->get_context(),
                std::move(element_clone),
                this->_initial_length
            );
        }

        [[nodiscard]]
        IAstType* get_element_type() const { return this->_element_type.get(); }

        [[nodiscard]]
        size_t get_initial_length() const { return this->_initial_length; }

        std::string to_string() override
        {
            return std::format(
                "Array[{}]{}",
                this->_element_type->to_string(),
                (this->get_flags() & SRFLAG_TYPE_OPTIONAL) != 0 ? "?" : ""
            );
        }

        [[nodiscard]]
        std::string get_internal_name() override
        {
            return std::format("[{}]", this->_element_type->get_internal_name());
        }

        bool equals(IAstType& other) override;
    };

    class AstFunctionType
        : public IAstType
    {
        std::vector<std::unique_ptr<IAstType>> _parameters;
        std::unique_ptr<IAstType> _return_type;

    public:
        AstFunctionType(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<IAstType>> parameters,
            std::unique_ptr<IAstType> return_type
        ) : IAstType(source, source_position, context, SRFLAG_TYPE_PTR),
            _parameters(std::move(parameters)),
            _return_type(std::move(return_type)) {}

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstType>>& get_parameters() const { return _parameters; }

        [[nodiscard]]
        const std::unique_ptr<IAstType>& get_return_type() const { return _return_type; }

        std::unique_ptr<IAstType> clone() const override
        {
            std::vector<std::unique_ptr<IAstType>> parameters_clone = {};
            for (const auto& p : this->_parameters)
            {
                parameters_clone.push_back(p->clone());
            }

            return std::make_unique<AstFunctionType>(
                this->get_source(),
                this->get_source_position(),
                this->get_context(),
                std::move(parameters_clone),
                this->_return_type->clone()
            );
        }

        std::string to_string() override
        {
            std::vector<std::string> param_strings;
            for (const auto& p : this->_parameters)
                param_strings.push_back(p->to_string());
            return std::format(
                "Function({}) -> {}",
                join(param_strings, ", "),
                this->_return_type->to_string()
            );
        }

        bool equals(IAstType& other) override
        {
            if (const auto* other_func = dynamic_cast<AstFunctionType*>(&other))
            {
                return this->_parameters == other_func->_parameters &&
                    this->_return_type->equals(*other_func->_return_type);
            }
            return false;
        }

        std::string get_internal_name() override
        {
            return "Function";
        }
    };

    std::unique_ptr<IAstType> parse_type(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        const std::string& error,
        int context_flags = SRFLAG_NONE
    );

    llvm::Type* internal_type_to_llvm_type(
        IAstType* type,
        llvm::Module* module
    );

    std::unique_ptr<IAstType> get_dominant_field_type(
        const std::shared_ptr<ParsingContext>& context,
        IAstType* lhs,
        IAstType* rhs
    );

    std::optional<std::unique_ptr<IAstType>> parse_primitive_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags = SRFLAG_NONE
    );

    std::optional<std::unique_ptr<IAstType>> parse_named_type_optional(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        int context_type_flags = SRFLAG_NONE
    );

    std::string get_root_reference_struct_name(
        const std::string& name,
        const std::shared_ptr<ParsingContext>& context
    );
}
