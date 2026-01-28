#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

std::unique_ptr<AstStructInitializer> stride::ast::parse_struct_initializer(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set
)
{
    const auto reference_token = set.peak_next();

    std::unordered_map<std::string, std::unique_ptr<AstExpression>> member_map = {};
    auto member_set = collect_block(set);

    if (!member_set.has_value())
    {
        set.throw_error("Expected struct initializer body after '{'");
    }

    while (member_set->has_next())
    {
        const auto member_iden = member_set.value().expect(
            TokenType::IDENTIFIER,
            "Expected identifier in struct initializer"
        );
        member_set.value().expect(TokenType::EQUALS, "Expected '=' after identifier in struct initializer");

        auto member_expr = parse_inline_expression(scope, member_set.value());

        member_map[member_iden.lexeme] = std::move(member_expr);
    }

    return std::make_unique<AstStructInitializer>(
        set.source(),
        reference_token.offset,
        scope,
        std::move(member_map)
    );
}

bool stride::ast::is_struct_initializer(const TokenSet& set)
{
    // We assume an expression is a struct initializer if it starts with `{ <identifier`
    // Obviously this can also be a block, but we will disambiguate that during parsing
    return set.peak_eq(TokenType::LBRACE, 0) && set.peak_eq(TokenType::IDENTIFIER, 1);
}

std::string AstStructInitializer::to_string()
{
    return std::format("StructInit{{...}}");
}

/*void AstStructInitializer::validate()
{
    // Check whether the struct we're trying to assign actually exists
}*/

llvm::Value* AstStructInitializer::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    // Collect member types by generating code for each member expression
    std::vector<llvm::Type*> member_types;
    std::vector<std::pair<std::string, llvm::Value*>> member_values;

    for (auto& [name, expr] : this->_initializers)
    {
        llvm::Value* value = expr->codegen(scope, module, context, builder);

        if (value == nullptr)
        {
            throw parsing_error(
                make_ast_error(
                    *this->source,
                    this->source_offset,
                    std::format("Failed to codegen struct member '{}'", name)
                )
            );
        }

        member_types.push_back(value->getType());
        member_values.emplace_back(name, value);
    }

    // Create struct type from member types
    llvm::StructType* struct_type = llvm::StructType::create(context, member_types, "struct.init");

    // Allocate struct on stack
    llvm::Value* struct_alloca = builder->CreateAlloca(struct_type, nullptr, "struct.tmp");

    // Store each member value into the struct
    for (size_t i = 0; i < member_values.size(); ++i)
    {
        llvm::Value* member_ptr = builder->CreateStructGEP(struct_type, struct_alloca, i);
        builder->CreateStore(member_values[i].second, member_ptr);
    }

    // Load and return the final struct value
    return builder->CreateLoad(struct_type, struct_alloca, "struct.val");
}
