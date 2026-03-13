#include "errors.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token_set.h"

#include <iostream>
#include <ranges>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

bool stride::ast::is_struct_initializer(const TokenSet& set)
{
    int64_t i = 0;

    if (!set.peek_eq(TokenType::IDENTIFIER, i))
    {
        return false;
    }
    ++i;

    // Optional generic argument list: SomeStruct<...>
    if (set.peek_eq(TokenType::LT, i))
    {
        int depth = 0;

        do
        {
            if (set.peek_eq(TokenType::LT, i))
            {
                ++depth;
            }
            else if (set.peek_eq(TokenType::GT, i))
            {
                --depth;
            }


            ++i;
            // Ran out of tokens before the generic list closed
            if (set.at(i).get_type() == TokenType::END_OF_FILE && depth > 0)
            {
                return false;
            }
        }
        while (depth > 0 && set.has_next());
    }

    return set.peek_eq(TokenType::DOUBLE_COLON, i)
        && set.peek_eq(TokenType::LBRACE, i + 1);
}

StructMemberInitializerPair parse_object_member_initializer(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto member_iden = set.expect(TokenType::IDENTIFIER, "Expected identifier in struct initializer");

    // Implicit reference to another variable with the same name as the member
    if (!set.has_next() || set.peek_next_eq(TokenType::COMMA))
    {
        auto member_symbol = Symbol(
            member_iden.get_source_fragment(),
            member_iden.get_lexeme()
        );
        return { member_iden.get_lexeme(), std::make_unique<AstIdentifier>(context, member_symbol) };
    }

    set.expect(TokenType::COLON, "Expected ':' after identifier in struct initializer");

    auto member_expr = parse_inline_expression(context, set);

    return { member_iden.get_lexeme(), std::move(member_expr) };
}


std::unique_ptr<AstObjectType> AstObjectInitializer::get_instantiated_object_type()
{
    if (this->_object_type != nullptr)
    {
        return this->_object_type->clone_as<AstObjectType>();
    }

    const auto type_def = this->get_context()->get_type_definition(this->_object_type_name);

    if (!type_def.has_value())
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Object type '{}' is undefined", this->_object_type_name),
            this->get_source_fragment()
        );
    }

    if (!this->_generic_type_arguments.empty() && !type_def.value()->get_generics_parameters().empty())
    {
        if (this->_generic_type_arguments.size() != type_def.value()->get_generics_parameters().size())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Invalid instantiation of object type '{}': expected {} generic arguments, got {}",
                            this->_object_type_name,
                            type_def.value()->get_generics_parameters().size(),
                            this->_generic_type_arguments.size()
                ),
                this->get_source_fragment()
            );
        }
    }

    if (const auto* object_def = cast_type<AstObjectType*>(type_def.value()->get_type()))
    {
        auto resolved_type = instantiate_generic_type(this, const_cast<AstObjectType*>(object_def), type_def.value());

        this->_object_type = std::move(resolved_type);

        return this->_object_type->clone_as<AstObjectType>();
    }

    if (auto* alias_def = cast_type<AstAliasType*>(type_def.value()->get_type()))
    {
        const auto underlying_type = alias_def->get_underlying_type();

        if (auto* object_def = cast_type<AstObjectType*>(underlying_type))
        {
            auto resolved_type = instantiate_generic_type(this, object_def, type_def.value());

            this->_object_type = std::move(resolved_type);

            return this->_object_type->clone_as<AstObjectType>();
        }
    }

    throw parsing_error(
        ErrorType::COMPILATION_ERROR,
        std::format("Type '{}' is not an object", this->_object_type_name),
        this->get_source_fragment()
    );
}

std::unique_ptr<AstObjectInitializer> stride::ast::parse_object_initializer(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.expect(TokenType::IDENTIFIER, "Expected object name in object initializer");
    auto generic_types = parse_generic_type_arguments(context, set);
    set.expect(TokenType::DOUBLE_COLON, "Expected '::' after object name in initialization");

    std::vector<StructMemberInitializerPair> member_map;
    auto member_set = collect_block_required(set, "Expected object initializer body after '{'");

    member_map.emplace_back(parse_object_member_initializer(context, member_set));

    // TODO: Handle unnamed initialization, e.g., `SomeStruct::{ 1, 3, 3 }`

    // Subsequent member parsing
    while (member_set.has_next())
    {
        member_set.expect(TokenType::COMMA, "Expected ',' between struct initializer members");

        // It's possible that this previous comma *was* the trailing one, so we'll have to do an
        // additional check
        if (!member_set.has_next())
        {
            break;
        }

        auto [member_iden, member_expr] =
            parse_object_member_initializer(context, member_set);
        member_map.emplace_back(std::move(member_iden), std::move(member_expr));
    }

    // Optionally consume trailing comma
    if (member_set.peek_next_eq(TokenType::COMMA))
    {
        member_set.next();
    }

    return std::make_unique<AstObjectInitializer>(
        reference_token.get_source_fragment(),
        context,
        reference_token.get_lexeme(),
        std::move(member_map),
        std::move(generic_types)
    );
}

void AstObjectInitializer::validate()
{
    const auto object_type = this->get_instantiated_object_type();

    const auto object_members = object_type->get_members();

    // Quick check: Ensure the number of members matches (no type comparisons required)
    if (object_members.size() != this->_member_initializers.size())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Too {} members found in object '{}': expected {}, got {}",
                object_members.size() > this->_member_initializers.size() ? "few" : "many",
                this->_object_type_name,
                object_members.size(),
                this->_member_initializers.size()),
            this->get_source_fragment()
        );
    }

    for (const auto& [field_name, initializer_expr] : this->_member_initializers)
    {
        initializer_expr->validate();
        auto member_type = object_type->get_member_field_type(field_name);

        if (!member_type.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Object type '{}' has no member named '{}'",
                    this->_object_type_name,
                    field_name
                ),
                this->get_source_fragment());
        }

        if (!initializer_expr->get_type()->equals(member_type.value()))
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Type mismatch for member '{}' in object initializer '{}': expected '{}', got '{}'",
                    field_name,
                    this->get_type()->to_string(),
                    member_type.value()->to_string(),
                    initializer_expr->get_type()->to_string()
                ),
                initializer_expr->get_source_fragment()
            );
        }

        // Further validate child nodes. It's possible that we have nested struct definitions,
        // which also have to conform to their types.
        initializer_expr->validate();
    }

    // Second quick check: Order validation - This is required to ensure a consistent data layout.
    size_t index = 0;
    for (const auto& member_name : this->_member_initializers | std::views::keys)
    {
        if (const auto& [field_name, field_type] = object_members[index];
            member_name != field_name)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Object member order mismatch at index {}: expected '{}', got '{}'",
                    index,
                    field_name,
                    member_name
                ),
                field_type->get_source_fragment()
            );
        }

        index++;
    }
}

void AstObjectInitializer::resolve_forward_references(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    for (const auto& val : this->_member_initializers | std::views::values)
    {
        val->resolve_forward_references(module, builder);
    }
}

llvm::Value* AstObjectInitializer::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    // Resolve member values
    std::vector<llvm::Constant*> constant_members;
    std::vector<llvm::Value*> dynamic_members;
    bool all_constants = true;

    for (const auto& expr : this->_member_initializers | std::views::values)
    {
        llvm::Value* val = expr->codegen(module, builder);
        if (!val)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "Failed to codegen member initializer",
                expr->get_source_fragment()
            );
        }

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
    const auto object_type = this->get_instantiated_object_type();

    auto* struct_type = llvm::cast<llvm::StructType>(object_type->get_llvm_type(module));

    if (!struct_type)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Struct type '{}' is undefined", this->_object_type_name),
            this->get_source_fragment()
        );
    }

    // Generate the struct value

    // CASE A: Global Variable Initialization (Requires Constant)
    // If all members are constants, we can create a ConstantStruct.
    // This resolves the "initializer type does not match" error for globals.
    if (all_constants)
    {
        std::vector<llvm::Constant*> casted_constant_members;
        casted_constant_members.reserve(constant_members.size());

        for (size_t i = 0; i < constant_members.size(); ++i)
        {
            auto* member_val = constant_members[i];
            auto* target_type = struct_type->getElementType(i);

            if (member_val->getType() != target_type)
            {
                if (member_val->getType()->isPointerTy() && target_type->isPointerTy())
                {
                    casted_constant_members.push_back(llvm::ConstantExpr::getBitCast(member_val, target_type));
                }
                else
                {
                    // FIXME:
                    // For non-pointers, if they are both structs but have different names, 
                    // LLVM doesn't allow bitcast. However, they should have been the same.
                    // If we reach here, we might need a more complex conversion or 
                    // there's a deeper type mismatch.
                    casted_constant_members.push_back(member_val);
                }
            }
            else
            {
                casted_constant_members.push_back(member_val);
            }
        }

        return llvm::ConstantStruct::get(struct_type, casted_constant_members);
    }

    // CASE B: Runtime Initialization (Function Body)
    // If we are checking logic that runs at runtime, we use InsertValue.
    llvm::Value* current_struct_val = llvm::UndefValue::get(struct_type);

    for (size_t i = 0; i < dynamic_members.size(); ++i)
    {
        auto* member_val = dynamic_members[i];

        if (auto* target_type = struct_type->getElementType(i);
            member_val->getType() != target_type)
        {
            if (member_val->getType()->isPointerTy() && target_type->isPointerTy())
            {
                member_val = builder->CreateBitCast(member_val, target_type);
            }
            // Note: We cannot bitcast between different struct types in LLVM if they are values.
            // If they are structurally identical but have different names, we might need to 
            // use a series of extractvalue/insertvalue or just bitcast the pointer if it was 
            // behind a pointer. But here they are values.
        }

        current_struct_val = builder->CreateInsertValue(
            current_struct_val,
            member_val,
            { static_cast<unsigned int>(i) },
            "object.construct"
        );
    }

    return current_struct_val;
}

std::unique_ptr<IAstNode> AstObjectInitializer::clone()
{
    std::vector<StructMemberInitializerPair> member_initializers;
    std::vector<std::unique_ptr<IAstType>> member_generic_types;

    member_initializers.reserve(this->_member_initializers.size());
    member_generic_types.reserve(this->_generic_type_arguments.size());

    for (const auto& [name, expr] : this->_member_initializers)
    {
        member_initializers.emplace_back(name, expr->clone_as<IAstExpression>());
    }

    for (const auto& type : this->_generic_type_arguments)
    {
        member_generic_types.push_back(type->clone_ty());
    }

    return std::make_unique<AstObjectInitializer>(
        this->get_source_fragment(),
        this->get_context(),
        this->_object_type_name,
        std::move(member_initializers),
        std::move(member_generic_types)
    );
}

std::string AstObjectInitializer::to_string()
{
    const auto object_name = this->_object_type_name;
    std::string members;

    for (const auto& [name, expr] : this->_member_initializers)
    {
        members += std::format("{}: {}, ", name, expr->to_string());
    }

    if (!this->_generic_type_arguments.empty())
    {
        std::vector<std::string> generic_names;
        for (const auto& generic : this->_generic_type_arguments)
        {
            generic_names.push_back(generic->to_string());
        }
        return std::format("(Object) {}<{}>{{ {} }}", object_name, join(generic_names, ", "), members);
    }

    return std::format("Object {}{{ {} }}", object_name, members);
}
