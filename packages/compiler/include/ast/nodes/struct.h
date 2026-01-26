#pragma once
#include <utility>

#include "ast_node.h"
#include "types.h"
#include "ast/symbol_registry.h"

namespace stride::ast
{
    class AstStructMember : public IAstNode
    {
        std::string _name;
        std::unique_ptr<IAstInternalFieldType> _type;

    public:
        AstStructMember(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<symbol_registry>& scope,
            std::string  name,
            std::unique_ptr<IAstInternalFieldType> type
        ) :
            IAstNode(source, source_offset, scope),
            _name(std::move(name)),
            _type(std::move(type)) {}

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return this->_name; }

        [[nodiscard]]
        IAstInternalFieldType& get_type() const { return *this->_type; }
    };

    class AstStruct : public IAstNode
    {
        std::string _name;
        std::vector<std::unique_ptr<AstStructMember>> _cases;

        // Whether this struct references another one
        // This can be used for declaring a type with the data layout
        // of another
        std::optional<std::unique_ptr<IAstInternalFieldType>> _reference;

    public:
        AstStruct(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<symbol_registry>& scope,
            std::string  name,
            std::unique_ptr<IAstInternalFieldType> reference
        )
            :
            IAstNode(source, source_offset, scope),
            _name(std::move(name)),
            _reference(std::move(reference)) {}

        AstStruct(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<symbol_registry>& scope,
            std::string  name,
            std::vector<std::unique_ptr<AstStructMember>> members
        ) :
            IAstNode(source, source_offset, scope),
            _name(std::move(name)),
            _cases(std::move(members)),
            _reference(std::nullopt) {}

        [[nodiscard]]
        bool is_reference_type() const { return _reference.has_value(); }

        [[nodiscard]]
        IAstInternalFieldType* get_reference_type() const { return _reference->get(); }

        std::string to_string() override;

        [[nodiscard]]
        std::string get_name() const { return _name; }

        [[nodiscard]]
        const std::vector<std::unique_ptr<AstStructMember>>& get_members() const { return this->_cases; }
    };

    std::unique_ptr<AstStruct> parse_struct_declaration(const std::shared_ptr<symbol_registry>& scope, TokenSet& tokens);

    bool is_struct_declaration(const TokenSet& tokens);
}
