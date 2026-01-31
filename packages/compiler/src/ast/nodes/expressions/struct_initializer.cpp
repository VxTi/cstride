#include <iostream>
#include <ranges>
#include <llvm/IR/Module.h>

#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

bool stride::ast::is_struct_initializer(const TokenSet& set)
{
    // We assume an expression is a struct initializer if it starts with `<name>::{ <identifier`
    // Obviously, this can also be a block, but we will disambiguate that during parsing
    return set.peek_eq(TokenType::IDENTIFIER, 0)
        && set.peek_eq(TokenType::DOUBLE_COLON, 1)
        && set.peek_eq(TokenType::LBRACE, 2);
}

std::pair<std::string, std::unique_ptr<AstExpression>> parse_struct_member_initializer(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    const auto member_iden = set.expect(
        TokenType::IDENTIFIER,
        "Expected identifier in struct initializer"
    );
    set.expect(TokenType::COLON, "Expected ':' after identifier in struct initializer");

    auto member_expr = parse_inline_expression(registry, set);

    return {member_iden.get_lexeme(), std::move(member_expr)};
}

std::unique_ptr<AstStructInitializer> stride::ast::parse_struct_initializer(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::IDENTIFIER, "Expected struct name in struct initializer");
    set.expect(TokenType::DOUBLE_COLON, "Expected '::' after struct name in struct initializer");

    std::vector<std::pair<std::string, std::unique_ptr<AstExpression>>> member_map = {};
    auto member_set = collect_block(set);

    if (!member_set.has_value())
    {
        set.throw_error("Expected struct initializer body after '{'");
    }

    // Parse initial member
    auto [initial_member_iden, initial_member_expr] = parse_struct_member_initializer(
        registry,
        member_set.value()
    );
    member_map.emplace_back(std::move(initial_member_iden), std::move(initial_member_expr));

    // Subsequent member parsing
    while (member_set->has_next())
    {
        member_set->expect(TokenType::COMMA, "Expected ',' between struct initializer members");

        // It's possible that this previous comma *was* the trailing one, so we'll have to do an additional check
        if (!member_set->has_next()) break;

        auto [member_iden, member_expr] = parse_struct_member_initializer(
            registry,
            member_set.value()
        );
        member_map.emplace_back(std::move(member_iden), std::move(member_expr));
    }

    // Optionally consume trailing comma
    if (member_set->peek_next_eq(TokenType::COMMA))
    {
        member_set->next();
    }

    return std::make_unique<AstStructInitializer>(
        set.get_source(),
        reference_token.get_source_position(),
        registry,
        reference_token.get_lexeme(),
        std::move(member_map)
    );
}

std::string AstStructInitializer::to_string()
{
    return std::format("StructInit{{...}}");
}

StructSymbolDef* get_super_referencing_struct_def(
    const std::shared_ptr<SymbolRegistry>& registry,
    const std::string& struct_name
)
{
    const auto definition = registry->get_struct_def(struct_name);

    if (!definition.has_value()) return nullptr;

    if (definition.value()->is_reference_struct())
    {
        return get_super_referencing_struct_def(registry, definition.value()->get_reference_struct_name().value());
    }

    return definition.value();
}

void AstStructInitializer::validate()
{
    const auto definition = get_super_referencing_struct_def(this->get_registry(), this->_struct_name);
    // Check whether the struct we're trying to assign actually exists
    if (definition == nullptr)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Struct '{}' does not exist", this->_struct_name),
            *this->get_source(),
            this->get_source_position()
        );
    }

    const auto fields = definition->get_fields();

    // Quick check: Ensure the number of members matches (no type comparisons required)
    if (fields.size() != this->_initializers.size())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Too {} members found in struct '{}': expected {}, got {}",
                fields.size() > this->_initializers.size() ? "few" : "many",
                this->_struct_name,
                fields.size(), this->_initializers.size()
            ),
            *this->get_source(),
            this->get_source_position()
        );
    }

    // Second quick check: Order validation - This is required to ensure consistent data layout.
    size_t index = 0;
    for (const auto& member_name : this->_initializers | std::views::keys)
    {
        if (const auto [field_name, field_type] = fields[index];
            member_name != field_name)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Struct member order mismatch at index {}: expected '{}', got '{}'",
                    index,
                    field_name,
                    member_name
                ),
                *this->get_source(),
                field_type->get_source_position()
            );
        }

        index++;
    }
    for (const auto& [member_name, member_expr] : this->_initializers)
    {
        const auto found_member = definition->get_field_type(member_name);

        if (!found_member.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Struct '{}' has no member named '{}'", this->_struct_name, member_name),
                *this->get_source(),
                this->get_source_position()
            );
        }

        if (const auto member_type = infer_expression_type(this->get_registry(), member_expr.get());
            !member_type->equals(*found_member.value()))
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Type mismatch for member '{}' in struct initializer '{}': expected '{}', got '{}'",
                    member_name,
                    this->_struct_name,
                    found_member.value()->to_string(),
                    member_type->to_string()
                ),
                *this->get_source(),
                member_expr->get_source_position()
            );
        }

        // Further validate child nodes. It's possible that we have nested struct definitions,
        // which also have to conform to their types.
        member_expr->validate();
    }
}

llvm::Value* AstStructInitializer::codegen(
    const std::shared_ptr<SymbolRegistry>& registry,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    // Resolve member values
    std::vector<llvm::Constant*> constant_members;
    std::vector<llvm::Value*> dynamic_members;
    bool all_constants = true;

    for (const auto& expr : this->_initializers | std::views::values)
    {
        llvm::Value* val = expr->codegen(registry, module, builder);
        if (!val) return nullptr;

        if (auto* c = llvm::dyn_cast<llvm::Constant>(val))
        {
            constant_members.push_back(c);
        }
        else
        {
            all_constants = false;
        }
        dynamic_members.push_back(val);
    }

    // Retrieve the exist named struct type
    auto struct_def_opt = registry->get_struct_def(this->_struct_name);
    std::string actual_struct_name = this->_struct_name;

    if (struct_def_opt.has_value())
    {
        auto struct_def = struct_def_opt.value();
        while (struct_def->is_reference_struct())
        {
            actual_struct_name = struct_def->get_reference_struct_name().value();
            struct_def_opt = registry->get_struct_def(actual_struct_name);
            if (!struct_def_opt.has_value()) break;
            struct_def = struct_def_opt.value();
        }
    }

    llvm::StructType* struct_type = llvm::StructType::getTypeByName(module->getContext(), actual_struct_name);

    if (!struct_type)
    {
        throw parsing_error(
            ErrorType::RUNTIME_ERROR,
            std::format("Struct type '{}' is undefined", this->_struct_name),
            *this->get_source(),
            this->get_source_position()
        );
    }

    // Generate the struct value

    // CASE A: Global Variable Initialization (Requires Constant)
    // If all members are constants, we can create a ConstantStruct.
    // This resolves the "initializer type does not match" error for globals.
    if (all_constants)
    {
        return llvm::ConstantStruct::get(struct_type, constant_members);
    }

    // CASE B: Runtime Initialization (Function Body)
    // If we are checking logic that runs at runtime, we use InsertValue.
    llvm::Value* current_struct_val = llvm::UndefValue::get(struct_type);

    for (size_t i = 0; i < dynamic_members.size(); ++i)
    {
        current_struct_val = builder->CreateInsertValue(
            current_struct_val,
            dynamic_members[i],
            {static_cast<unsigned int>(i)},
            "struct.build"
        );
    }

    return current_struct_val;
}
