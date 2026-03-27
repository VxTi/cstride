#pragma once

#include "ast_node.h"
#include "types.h"

#include <format>
#include <utility>

namespace stride::ast
{
    enum class VisibilityModifier;
    class TokenSet;

    class AstTypeDefinition
        : public IAstNode
    {
        std::string _name;
        std::unique_ptr<IAstType> _type;
        VisibilityModifier _visibility;
        GenericParameterList _generic_parameters;

    public:
        explicit AstTypeDefinition(
            const SourcePosition& source,
            std::string name,
            std::unique_ptr<IAstType> type,
            const VisibilityModifier visibility,
            GenericParameterList generic_parameters = {}
        ) :
            IAstNode(source),
            _name(std::move(name)),
            _type(std::move(type)),
            _visibility(visibility),
            _generic_parameters(std::move(generic_parameters)) {}

        [[nodiscard]]
        const std::string& get_name() const
        {
            return this->_name;
        }

        [[nodiscard]]
        IAstType* get_type() const
        {
            return this->_type.get();
        }

        [[nodiscard]]
        const VisibilityModifier& get_visibility() const
        {
            return this->_visibility;
        }

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override
        {
            return nullptr;
        }

        std::unique_ptr<IAstNode> clone() override;

        [[nodiscard]]
        const GenericParameterList& get_generic_parameters() const
        {
            return this->_generic_parameters;
        }

        [[nodiscard]]
        bool is_generic_type() const
        {
            return !this->_generic_parameters.empty();
        }

        std::string to_string() override
        {
            return std::format("Type<{}>", this->get_name());
        }
    };

    std::unique_ptr<AstTypeDefinition> parse_type_definition(TokenSet& set, VisibilityModifier modifier);

    EnumMemberPair parse_enumerable_member(TokenSet& set, size_t element_index);

    std::unique_ptr<AstTypeDefinition> parse_enum_type_definition(TokenSet& set, VisibilityModifier modifier);
}
