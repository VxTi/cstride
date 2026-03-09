#include "ast/casting.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/DerivedTypes.h>

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_function_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    int context_type_flags
)
{
    // Must start with '('
    if (!set.peek_next_eq(TokenType::LPAREN))
    {
        return std::nullopt;
    }

    // This tries to parse `(<type>, <type>, ...) -> <return_type>`
    // With no parameters: `() -> <return_type>`
    // Arrays: ((<type>, ...) -> <return_type>)[]

    const auto reference_token = set.next();
    std::vector<std::unique_ptr<IAstType>> parameters;
    bool is_expecting_closing_paren = false;

    // If the previous paren is followed by another one,
    // then we might expect an array of functions
    // e.g., `((i32, i32) -> i32)[]
    if (set.peek_next_eq(TokenType::LPAREN))
    {
        set.next();
        is_expecting_closing_paren = true;
    }

    while (set.has_next() && !set.peek_next_eq(TokenType::RPAREN))
    {
        parameters.push_back(
            parse_type(
                context,
                set,
                "Expected parameter type",
                context_type_flags
            )
        );
        if (set.peek_next_eq(TokenType::RPAREN))
        {
            break;
        }
        set.expect(TokenType::COMMA, "Expected ',' between function type parameters");
    }

    set.expect(TokenType::RPAREN, "Expected ')' after function type notation");
    set.expect(TokenType::RARROW, "Expected '->' between function parameters and return type");
    auto return_type = parse_type(
        context,
        set,
        "Expected return type",
        context_type_flags
    );

    if (is_expecting_closing_paren)
    {
        set.expect(TokenType::RPAREN, "Expected secondary ')' after function type notation");
    }

    auto fn_type = std::make_unique<AstFunctionType>(
        reference_token.get_source_fragment(),
        context,
        std::move(parameters),
        std::move(return_type),
        context_type_flags
    );

    return parse_type_metadata(std::move(fn_type), set, context_type_flags);
}

llvm::FunctionType* AstFunctionType::get_llvm_type(llvm::Module* module) const
{
    std::vector<llvm::Type*> param_types;
    param_types.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        param_types.push_back(type_to_llvm_type(param.get(), module));
    }

    llvm::Type* ret_type = type_to_llvm_type(
        this->_return_type.get(),
        module
    );
    return llvm::FunctionType::get(
        ret_type,
        param_types,
        false
    );
}

std::unique_ptr<IAstNode> AstFunctionType::clone()
{
    std::vector<std::unique_ptr<IAstType>> parameters;
    parameters.reserve(this->_parameters.size());
    for (const auto& p : this->_parameters)
    {
        parameters.push_back(p->clone_ty());
    }

    return std::make_unique<AstFunctionType>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(parameters),
        this->_return_type->clone_ty(),
        this->get_flags()
    );
}

bool AstFunctionType::equals(const IAstType& other) const
{
    if (const auto* other_func = cast_type<const AstFunctionType*>(&other))
    {
        if (!other_func->get_return_type()->equals(
            *this->get_return_type()))
            return false;

        if (this->_parameters.size() != other_func->_parameters.size())
            return false;

        for (size_t i = 0; i < this->_parameters.size(); i++)
        {
            if (!this->_parameters[i]->equals(
                *other_func->_parameters[i]))
            {
                return false;
            }
        }
        return true;
    }

    if (const auto* other_named = cast_type<const AstAliasType*>(&other))
    {
        return other_named->equals(*this);
    }

    return false;
}

bool AstFunctionType::is_castable_to_impl(IAstType* other)
{
    if (const auto other_named = cast_type<AstAliasType*>(other))
    {
        if (const auto base_type = other_named->get_underlying_type(); base_type.has_value())
        {
            return this->equals(*base_type.value());
        }
    }

    return false;
}

std::string AstFunctionType::get_type_name()
{
    std::vector<std::string> param_strings;
    param_strings.reserve(this->_parameters.size());

    for (const auto& p : this->_parameters)
        param_strings.push_back(p->to_string());

    return std::format(
        "({}) -> {}",
        join(param_strings, ", "),
        this->_return_type->to_string());
}
