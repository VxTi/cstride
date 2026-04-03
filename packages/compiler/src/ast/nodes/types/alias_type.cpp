#include "ast/casting.h"
#include "ast/symbol_table.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_alias_type_optional(
    TokenSet& set,
    const TypeParsingOptions& options
)
{
    if (!set.peek_next_eq(TokenType::IDENTIFIER))
        return std::nullopt;

    int flags = options.flags;

    // Parses an alias type instantiation, like `Module::SomeType<i32>`
    //
    // - Parses `Module::SomeType`
    const auto identifier_name = parse_segmented_identifier(set, "Expected identifier for named type");
    // - Parses `<i32>`
    auto generic_types = parse_generic_type_arguments(set);

    return parse_type_metadata(
        std::make_unique<AstAliasType>(
            identifier_name->get_source_position(),
            identifier_name->get_scoped_name(),
            flags,
            std::move(generic_types)
        ),
        set
    );
}

static std::unique_ptr<IAstType> resolve_nested_underlying_types(std::unique_ptr<IAstType> type, int recursion_guard)
{
    if (++recursion_guard > MAX_RECURSION_DEPTH)
    {
        throw stride::stride_error(
            stride::ErrorType::COMPILATION_ERROR,
            "Exceeded maximum recursion depth while resolving nested underlying types",
            type->get_source_position()
        );
    }

    if (cast_type<AstAliasType*>(type.get()))
    {
        // Don't eagerly resolve alias types here — they may be self-referencing
        // (e.g., Promise<T> = { then: () -> Promise<T> }) and would cause infinite
        // recursion. Leave them as aliases to be lazily resolved when accessed.
        return std::move(type);
    }

    if (const auto* array = cast_type<AstArrayType*>(type.get()))
    {
        auto element_type = array->get_element_type()->clone();
        auto resolved_element = resolve_nested_underlying_types(std::move(element_type), recursion_guard);

        return std::make_unique<AstArrayType>(
            array->get_source_position(),
            std::move(resolved_element),
            array->get_initial_length(),
            array->get_flags()
        );
    }

    if (const auto* object_type = cast_type<AstObjectType*>(type.get()))
    {
        ObjectTypeMemberList resolved_members;
        for (const auto& [name, member_type] : object_type->get_members())
        {
            resolved_members.emplace_back(
                name,
                resolve_nested_underlying_types(member_type->clone(), recursion_guard)
            );
        }

        GenericTypeList resolved_generics;
        for (const auto& gen : object_type->get_instantiated_generics())
        {
            resolved_generics.push_back(resolve_nested_underlying_types(gen->clone(), recursion_guard));
        }

        return std::make_unique<AstObjectType>(
            object_type->get_source_position(),
            object_type->get_base_name(),
            std::move(resolved_members),
            object_type->get_flags(),
            std::move(resolved_generics)
        );
    }

    if (const auto* tuple = cast_type<AstTupleType*>(type.get()))
    {
        std::vector<std::unique_ptr<IAstType>> resolved_members;
        for (const auto& member : tuple->get_members())
        {
            resolved_members.push_back(resolve_nested_underlying_types(member->clone(), recursion_guard));
        }

        return std::make_unique<AstTupleType>(
            tuple->get_source_position(),
            std::move(resolved_members),
            tuple->get_flags()
        );
    }

    if (const auto* func = cast_type<AstFunctionType*>(type.get()))
    {
        std::vector<std::unique_ptr<IAstType>> resolved_params;
        for (const auto& param : func->get_parameter_types())
        {
            resolved_params.push_back(resolve_nested_underlying_types(param->clone(), recursion_guard));
        }

        auto resolved_return = resolve_nested_underlying_types(func->get_return_type()->clone(), recursion_guard);

        return std::make_unique<AstFunctionType>(
            func->get_source_position(),
            std::move(resolved_params),
            std::move(resolved_return),
            func->get_generic_parameter_names(),
            func->get_flags()
        );
    }

    return std::move(type);
}

void AstAliasType::resolve_type_definition(const SymbolTable* symbol_table)
{
    if (this->_type_definition != nullptr)
        return;

    if (const auto ref_def = symbol_table->get_type_definition(this->get_name());
        ref_def.has_value())
    {
        this->_type_definition = ref_def.value();
        return;
    }

    throw stride_error(
        ErrorType::COMPILATION_ERROR,
        std::format(
            "Could not find definition for type '{}'",
            this->get_name()),
        this->get_source_position()
    );
}

IAstType* AstAliasType::get_underlying_type()
{
    if (this->_underlying_type)
    {
        return this->_underlying_type.get();
    }

    const auto& type_def = this->get_type_definition();

    std::unique_ptr<IAstType> base_type = this->is_generic_overload()
        ? instantiate_generic_type(this, type_def)
        : this->get_reference_type().value_or(nullptr);

    if (base_type == nullptr)
    {
        throw stride_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Could not find underlying type for type '{}'",
                this->get_name()),
            this->get_source_position()
        );
    }

    int recursion_guard = 0;

    while (true)
    {
        if (++recursion_guard > MAX_RECURSION_DEPTH)
        {
            throw stride_error(
                ErrorType::COMPILATION_ERROR,
                std::format(
                    "Exceeded maximum recursion depth while resolving base type of '{}'",
                    this->get_name()),
                this->get_source_position()
            );
        }

        if (const auto* named_reference = cast_type<AstAliasType*>(base_type.get()))
        {
            if (named_reference->is_generic_overload())
            {
                if (const auto next_def = named_reference->get_type_definition())
                {
                    base_type = instantiate_generic_type(named_reference, next_def);
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (auto next_type = named_reference->get_reference_type();
                    next_type.has_value())
                {
                    base_type = std::move(next_type.value());
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            // If it's not a named type, it might still contain named types (like Wrap<T>[])
            base_type = resolve_nested_underlying_types(std::move(base_type), recursion_guard);

            if (cast_type<AstAliasType*>(base_type.get()))
            {
                continue;
            }
            break;
        }
    }

    if (!base_type)
    {
        throw stride_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "Could not resolve underlying type for type '{}'",
                this->get_name()),
            this->get_source_position()
        );
    }

    this->_underlying_type = std::move(base_type);

    return this->_underlying_type.get();
}

bool AstAliasType::is_castable_to_impl(IAstType* other)
{
    const auto self_reference_type = get_underlying_type();

    // Check our base type is a primitive, and whether that type is castable to `other`
    if (auto* other_primitive = cast_type<AstPrimitiveType*>(other))
    {
        return self_reference_type->is_castable_to(other_primitive);
    }

    // A named type should be castable to its base type and vice versa
    if (self_reference_type->is_castable_to(other))
    {
        return true;
    }

    // Final case would be to check whether both base types are the same
    if (auto* other_alias_ty = cast_type<AstAliasType*>(other))
    {
        if (const auto second_reference_type = other_alias_ty->get_underlying_type())
        {
            return self_reference_type->equals(second_reference_type);
        }
    }
    return false;
}

bool AstAliasType::is_assignable_to_impl(IAstType* other)
{
    // Basic name comparison is already done higher up.
    // Here we check other conditions that might have to be met to assign.

    // It might be the case that we're trying to assign primitive references to a named value, e.g.,
    // type SomePrimitive = i32[]
    // const someVar: SomePrimitive = [1, 2, 3];
    // In this case, `[1, 2, 3]` should be assignable to the base types of `SomePrimitive`
    if (const auto self_base_type = get_underlying_type())
    {
        return self_base_type->is_assignable_to(other);
    }

    return false;
}

llvm::Type* AstAliasType::get_llvm_type_impl(llvm::Module* module)
{
    return this->get_underlying_type()->get_llvm_type(module);
}

bool AstAliasType::equals(IAstType* other)
{
    // Simple naming checks, e.g., "Vec3 == Vec3"
    if (auto* other_named = cast_type<const AstAliasType*>(other))
    {
        if (this->get_name() != other_named->get_name())
            return false;

        // If both aren't generic, then name comparison should suffice
        if (!this->is_generic_overload() && !other_named->is_generic_overload())
        {
            return true;
        }

        const auto& self_generic_types = this->get_instantiated_generic_types();
        const auto& other_generic_types = other_named->get_instantiated_generic_types();

        if (self_generic_types.size() != other_generic_types.size())
            return false;

        // If both are generic, they must have the same number of parameters
        for (size_t i = 0; i < self_generic_types.size(); i++)
        {
            if (!self_generic_types[i]->equals(other_generic_types[i].get()))
            {
                return false;
            }
        }
        return true;
    }

    if (const auto self_base = this->get_underlying_type())
    {
        return self_base->equals(other);
    }

    return false;
}

std::string AstAliasType::get_type_name()
{
    if (!this->_generic_types.empty())
    {
        std::vector<std::string> generic_names;
        for (const auto& generic : this->_generic_types)
        {
            generic_names.push_back(generic->get_type_name());
        }

        return std::format("{}<{}>", this->_name, join(generic_names, ", "));
    }
    return this->_name;
}

std::unique_ptr<IAstType> AstAliasType::clone()
{
    GenericTypeList generic_types;
    generic_types.reserve(this->_generic_types.size());
    for (const auto& generic_type : this->_generic_types)
    {
        generic_types.push_back(generic_type->clone());
    }
    return std::make_unique<AstAliasType>(
        this->get_source_position(),
        this->_name,
        this->get_flags(),
        std::move(generic_types)
    );
}
