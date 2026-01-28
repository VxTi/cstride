#include <ranges>

#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

bool stride::ast::is_struct_initializer(const TokenSet& set)
{
    // We assume an expression is a struct initializer if it starts with `<name>::{ <identifier`
    // Obviously, this can also be a block, but we will disambiguate that during parsing
    return set.peak_eq(TokenType::IDENTIFIER, 0)
        && set.peak_eq(TokenType::DOUBLE_COLON, 1)
        && set.peak_eq(TokenType::LBRACE, 2);
}

std::pair<std::string, std::unique_ptr<AstExpression>> parse_struct_member_initializer(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto member_iden = set.expect(
        TokenType::IDENTIFIER,
        "Expected identifier in struct initializer"
    );
    set.expect(TokenType::COLON, "Expected ':' after identifier in struct initializer");

    auto member_expr = parse_inline_expression(scope, set);

    return {member_iden.lexeme, std::move(member_expr)};
}

std::unique_ptr<AstStructInitializer> stride::ast::parse_struct_initializer(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::IDENTIFIER, "Expected struct name in struct initializer");
    set.expect(TokenType::DOUBLE_COLON, "Expected '::' after struct name in struct initializer");

    std::unordered_map<std::string, std::unique_ptr<AstExpression>> member_map = {};
    auto member_set = collect_block(set);

    if (!member_set.has_value())
    {
        set.throw_error("Expected struct initializer body after '{'");
    }

    // Parse initial member
    auto [initial_member_iden, initial_member_expr] = parse_struct_member_initializer(
        scope,
        member_set.value()
    );
    member_map[initial_member_iden] = std::move(initial_member_expr);

    while (member_set->has_next())
    {
        member_set->expect(TokenType::COMMA, "Expected ',' between struct initializer members");

        // It's possible that this previous comma *was* the trailing one, so we'll have to do an additional check
        if (!member_set->has_next()) break;

        auto [member_iden, member_expr] = parse_struct_member_initializer(
            scope,
            member_set.value()
        );
        member_map[member_iden] = std::move(member_expr);
    }

    // Optionally consume trailing comma
    if (member_set->peak_next_eq(TokenType::COMMA))
    {
        member_set->next();
    }

    return std::make_unique<AstStructInitializer>(
        set.source(),
        reference_token.offset,
        scope,
        reference_token.lexeme,
        std::move(member_map)
    );
}

std::string AstStructInitializer::to_string()
{
    return std::format("StructInit{{...}}");
}

void AstStructInitializer::validate()
{
    // Check whether the struct we're trying to assign actually exists
    if (this->scope->get_struct_def(this->_struct_name) == nullptr)
    {
        throw parsing_error(make_ast_error(
            *this->source,
            this->source_offset,
            std::format("Struct '{}' does not exist", this->_struct_name)
        ));
    }
}

llvm::Value* AstStructInitializer::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    // 1. Resolve member values
    std::vector<llvm::Constant*> constant_members;
    std::vector<llvm::Value*> dynamic_members;
    bool all_constants = true;

    for (const auto& expr : this->_initializers | std::views::values)
    {
        llvm::Value* val = expr->codegen(scope, module, context, builder);
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
    llvm::StructType* struct_type = llvm::StructType::getTypeByName(context, this->_struct_name);

    if (!struct_type)
    {
        throw parsing_error(make_ast_error(
            *this->source,
            this->source_offset,
            std::format("Struct type '{}' is undefined", this->_struct_name)
        ));
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
