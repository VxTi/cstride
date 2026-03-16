#pragma once

#include "ast/parsing_context.h"
#include "ast/nodes/function_definition.h"

#include <vector>

namespace stride::ast::definition
{
    struct GenericFunctionOverload
    {
        mutable llvm::Function* function;
        std::unique_ptr<AstFunctionDeclaration> node;
    };

    class FunctionDefinition
        : public IDefinition
    {
        std::unique_ptr<AstFunctionType> _function_type;
        std::vector<GenericFunctionOverload> _function_candidates{};
        int _flags;

        llvm::Function* _llvm_function = nullptr;

    public:
        explicit FunctionDefinition(
            std::unique_ptr<AstFunctionType> function_type,
            const Symbol& symbol,
            const VisibilityModifier visibility,
            const int flags
        ) :
            IDefinition(symbol, visibility),
            _function_type(std::move(function_type)),
            _flags(flags) {}

        [[nodiscard]]
        AstFunctionType* get_type() const
        {
            return this->_function_type.get();
        }

        [[nodiscard]]
        std::string get_function_name() const
        {
            return this->get_symbol().name;
        }

        [[nodiscard]]
        int get_flags() const
        {
            return this->_flags;
        }

        [[nodiscard]]
        bool is_variadic() const
        {
            return (this->_flags & SRFLAG_FN_TYPE_VARIADIC) != 0;
        }

        void add_generic_instantiation(GenericTypeList generic_overload_types);

        [[nodiscard]]
        const std::vector<GenericFunctionOverload>& get_instantiations() const
        {
            return this->_function_candidates;
        }

        [[nodiscard]]
        bool has_generic_instantiation(const GenericTypeList& generic_types) const;

        ~FunctionDefinition() override = default;

        bool matches_type_signature(const std::string& name, const AstFunctionType* signature) const;

        void set_llvm_function(llvm::Function* function)
        {
            this->_llvm_function = function;
        }

        [[nodiscard]]
        llvm::Function* get_llvm_function() const
        {
            return this->_llvm_function;
        }

        [[nodiscard]]
        bool matches_parameter_signature(
            const std::string& internal_function_name,
            const std::vector<std::unique_ptr<IAstType>>& other_parameter_types,
            size_t generic_argument_count
        ) const;

        [[nodiscard]]
        std::unique_ptr<IDefinition> clone() const override
        {
            return std::make_unique<FunctionDefinition>(
                _function_type->clone_as<AstFunctionType>(),
                get_symbol(),
                get_visibility(),
                _flags);
        }
    };
}
