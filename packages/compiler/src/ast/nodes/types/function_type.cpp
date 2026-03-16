#include "ast/casting.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/DerivedTypes.h>

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_function_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    const TypeParsingOptions& options
)
{
    int flags = options.flags;
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
                { "Expected parameter type", options.type_name, flags }
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
        { "Expected return type", options.type_name, flags }
    );

    if (is_expecting_closing_paren)
    {
        set.expect(TokenType::RPAREN, "Expected secondary ')' after function type notation");
    }

    // TODO: Resolve generic parameters in type resolution

    return parse_type_metadata(
        std::make_unique<AstFunctionType>(
            reference_token.get_source_fragment(),
            context,
            std::move(parameters),
            std::move(return_type),
            EMPTY_GENERIC_PARAMETER_LIST,
            flags
        ),
        set
    );
}

std::unique_ptr<IAstNode> AstFunctionType::clone()
{
    std::vector<std::unique_ptr<IAstType>> parameters;
    GenericParameterList generic_parameters_clone;

    parameters.reserve(this->_parameters.size());
    generic_parameters_clone.reserve(this->_generic_param_names.size());

    for (const auto& p : this->_parameters)
    {
        parameters.push_back(p->clone_ty());
    }

    generic_parameters_clone.insert(
        generic_parameters_clone.end(),
        this->_generic_param_names.begin(),
        this->_generic_param_names.end());

    return std::make_unique<AstFunctionType>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(parameters),
        this->get_return_type()->clone_ty(),
        std::move(generic_parameters_clone),
        this->get_flags()
    );
}

bool AstFunctionType::equals(IAstType* other)
{
    if (const auto* other_func = cast_type<const AstFunctionType*>(other))
    {
        if (!other_func->get_return_type()->equals(this->get_return_type().get()))
            return false;

        if (this->_parameters.size() != other_func->_parameters.size())
            return false;

        for (size_t i = 0; i < this->_parameters.size(); i++)
        {
            if (!this->_parameters[i]->equals(other_func->_parameters[i].get()))
            {
                return false;
            }
        }
        return true;
    }

    if (auto* other_named = dynamic_cast<AstAliasType*>(other))
    {
        return other_named->equals(this);
    }

    return false;
}

bool AstFunctionType::is_castable_to_impl(IAstType* other)
{
    if (const auto other_named = cast_type<AstAliasType*>(other))
    {
        return this->equals(other_named->get_underlying_type());
    }

    return false;
}

llvm::Type* AstFunctionType::get_llvm_type_impl(llvm::Module* module)
{
    std::vector<llvm::Type*> param_types;
    param_types.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        param_types.push_back(param->get_llvm_type(module));
    }

    llvm::Type* ret_type = this->_return_type->get_llvm_type(module);
    return llvm::FunctionType::get(
        ret_type,
        param_types,
        this->is_variadic()
    );
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
