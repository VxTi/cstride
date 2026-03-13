#include "errors.h"
#include "formatting.h"
#include "ast/casting.h"
#include "ast/closures.h"
#include "ast/parsing_context.h"
#include "ast/nodes/enumerables.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/blocks.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

/// Checks whether the next two tokens begin a member access (`.identifier`).
bool stride::ast::is_member_accessor(const TokenSet& set)
{
    return set.peek_eq(TokenType::DOT, 0)
        && set.peek_eq(TokenType::IDENTIFIER, 1);
}

/// Consumes `.identifier` and wraps `lhs` in an AstChainedExpression.
std::unique_ptr<AstChainedExpression> stride::ast::parse_chained_member_access(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<IAstExpression> lhs
)
{
    set.expect(TokenType::DOT, "Expected '.' in member access");
    const auto member_tok = set.expect(TokenType::IDENTIFIER, "Expected identifier after '.' in member access");

    auto member_id = std::make_unique<AstIdentifier>(
        context,
        Symbol(member_tok.get_source_fragment(), member_tok.get_lexeme())
    );

    const auto source = SourceFragment::combine(lhs->get_source_fragment(), member_tok.get_source_fragment());

    return std::make_unique<AstChainedExpression>(
        source,
        context,
        std::move(lhs),
        std::move(member_id)
    );
}

/// Consumes `(<args>)` and wraps `callee` in an AstIndirectCall.
std::unique_ptr<AstIndirectCall> stride::ast::parse_indirect_call(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::unique_ptr<IAstExpression> callee
)
{
    const auto callee_src = callee->get_source_fragment();
    auto param_block = collect_parenthesized_block(set);

    ExpressionList args;
    if (param_block.has_value())
    {
        auto subset = param_block.value();
        if (subset.has_next())
        {
            args.push_back(parse_inline_expression(context, subset));
            while (subset.has_next())
            {
                subset.expect(TokenType::COMMA, "Expected ',' between arguments");
                args.push_back(parse_inline_expression(context, subset));
            }
        }
    }

    const auto close_src = set.peek(-1).get_source_fragment();
    const auto source = SourceFragment::combine(callee_src, close_src);

    return std::make_unique<AstIndirectCall>(
        source,
        context,
        std::move(callee),
        std::move(args)
    );
}

llvm::Value* AstChainedExpression::codegen_global_member_accessor(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
) const
{
    llvm::Value* base_val = this->_base->codegen(module, builder);
    IAstType* current_type = this->_base->get_type();
    std::string current_struct_name = current_type->get_type_name();

    auto* global_var = llvm::dyn_cast_or_null<llvm::GlobalVariable>(base_val);
    if (!global_var || !global_var->hasInitializer())
    {
        return nullptr;
    }

    llvm::Constant* current_const = global_var->getInitializer();

    const auto* member_id = cast_expr<AstIdentifier*>(this->_followup.get());
    if (!member_id)
    {
        return nullptr;
    }

    auto struct_def_opt = this->get_context()->get_object_type(current_struct_name);
    if (!struct_def_opt.has_value())
        return nullptr;

    const auto struct_def = struct_def_opt.value();
    const auto member_index = struct_def->get_member_field_index(member_id->get_name());
    if (!member_index.has_value())
        return nullptr;

    current_const = current_const->getAggregateElement(member_index.value());
    if (!current_const)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format("Invalid member access on constant '{}'", current_struct_name),
            this->get_source_fragment()
        );
    }

    return current_const;
}

llvm::Value* AstChainedExpression::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    if (!builder->GetInsertBlock())
    {
        return codegen_global_member_accessor(module, builder);
    }

    llvm::Value* current_val = this->_base->codegen(module, builder);
    if (!current_val)
    {
        return nullptr;
    }

    const auto base_struct_type = get_object_type_from_type(this->_base->get_type());
    if (!base_struct_type.has_value())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be a struct type",
            this->get_source_fragment()
        );
    }

    const auto* member_id = cast_expr<AstIdentifier*>(this->_followup.get());
    if (!member_id)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Chained expression followup must be an identifier (member name)",
            this->get_source_fragment()
        );
    }

    IAstType* parent_type = base_struct_type.value();
    const std::string parent_struct_internalized_name = base_struct_type.value()->get_internalized_name();

    const bool is_pointer_ty = current_val->getType()->isPointerTy();

    auto parent_struct_type_opt = get_object_type_from_type(parent_type);
    if (!parent_struct_type_opt.has_value())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Cannot access member '{}' of non-struct type",
                member_id->get_name()
            ),
            this->get_source_fragment()
        );
    }

    const auto parent_struct_type = parent_struct_type_opt.value();
    const std::string internalized_name = parent_struct_type->get_internalized_name();

    const auto member_index = parent_struct_type->get_member_field_index(member_id->get_name());
    if (!member_index.has_value())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format(
                "Cannot access member '{}' of object type '{}': member does not exist",
                member_id->get_name(),
                parent_struct_type->get_type_name()
            ),
            this->get_source_fragment()
        );
    }

    if (is_pointer_ty)
    {
        llvm::StructType* struct_llvm_type = llvm::StructType::getTypeByName(
            module->getContext(),
            internalized_name
        );
        if (!struct_llvm_type)
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Object '{}' not registered internally", parent_struct_type->get_type_name()),
                this->get_source_fragment()
            );
        }

        llvm::Value* member_ptr = builder->CreateStructGEP(
            struct_llvm_type,
            current_val,
            member_index.value(),
            "ptr_" + member_id->get_name()
        );

        auto member_field_type = parent_struct_type->get_member_field_type(member_id->get_name());
        if (!member_field_type.has_value())
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Unknown member type '{}' in object '{}'",
                    member_id->get_name(), parent_struct_type->get_type_name()),
                this->get_source_fragment()
            );
        }

        llvm::Type* final_llvm_type = member_field_type.value()->get_llvm_type(module);
        return builder->CreateLoad(final_llvm_type, member_ptr, "val_member_access");
    }

    // Value (not pointer) — use ExtractValue
    llvm::Value* val = builder->CreateExtractValue(
        current_val,
        member_index.value(),
        "val_" + member_id->get_name()
    );

    return val;
}

std::unique_ptr<IAstNode> AstChainedExpression::clone()
{
    return std::make_unique<AstChainedExpression>(
        this->get_source_fragment(),
        this->get_context(),
        this->_base->clone_as<IAstExpression>(),
        this->_followup->clone_as<IAstExpression>()
    );
}

void AstChainedExpression::validate()
{
    this->_base->validate();
    this->_followup->validate();
}

std::string AstChainedExpression::to_string()
{
    return std::format(
        "ChainedExpression(base: {}, followup: {})",
        this->_base->to_string(),
        this->_followup->to_string()
    );
}

std::optional<std::unique_ptr<IAstNode>> AstChainedExpression::reduce()
{
    return std::nullopt;
}

bool AstChainedExpression::is_reducible()
{
    return false;
}

llvm::Value* AstIndirectCall::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Value* callee_val = this->get_callee()->codegen(module, builder);
    if (!callee_val)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            "Indirect call: could not evaluate callee expression",
            this->get_source_fragment()
        );
    }

    // Derive the LLVM function type from the callee's AST type
    auto callee_ast_type = this->get_callee()->get_type()->clone_ty();
    if (auto* alias_ty = cast_type<AstAliasType*>(callee_ast_type.get()))
    {
        callee_ast_type = alias_ty->get_underlying_type()->clone_ty();
    }

    const auto* fn_type = dynamic_cast<AstFunctionType*>(callee_ast_type.get());
    if (!fn_type)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Indirect call: callee expression does not have a function type",
            this->get_source_fragment()
        );
    }

    std::vector<llvm::Type*> llvm_param_types;
    llvm_param_types.reserve(fn_type->get_parameter_types().size());
    for (const auto& param : fn_type->get_parameter_types())
    {
        llvm_param_types.push_back(param->get_llvm_type(module));
    }

    llvm::FunctionType* llvm_fn_type = llvm::FunctionType::get(
        fn_type->get_return_type()->get_llvm_type(module),
        llvm_param_types,
        fn_type->is_variadic()
    );

    std::vector<llvm::Value*> args_v;
    llvm::FunctionType* call_fn_type = llvm_fn_type;
    llvm::Value* actual_fn_ptr = callee_val;

    // When the callee is a struct field access, the value is likely a closure
    // env ptr (heap-allocated {fn_ptr, captures...}). Prefer the lambda with
    // captures so we extract them correctly. For other callees (e.g. return
    // values from function calls), prefer the exact-match lambda.
    const bool callee_is_field_access =
        cast_expr<AstChainedExpression*>(this->get_callee()) != nullptr;

    // Check if this is a closure call that needs capture extraction
    if (llvm::Function* lambda_fn =
        closures::find_lambda_function(module, llvm_fn_type, callee_is_field_access))
    {
        const size_t num_captures = lambda_fn->arg_size()
            - fn_type->get_parameter_types().size();

        if (num_captures > 0)
        {
            auto capture_args = closures::extract_closure_captures(
                module, builder, callee_val, lambda_fn);

            if (capture_args.size() == num_captures)
            {
                call_fn_type = lambda_fn->getFunctionType();

                // Extract the actual function pointer from offset 0 of the closure env
                actual_fn_ptr = builder->CreateLoad(
                    lambda_fn->getType(),
                    callee_val,
                    "closure_fn_ptr"
                );

                args_v.insert(args_v.end(), capture_args.begin(), capture_args.end());
            }
        }
    }

    // Add the user-provided arguments
    for (const auto& arg : this->get_args())
    {
        llvm::Value* arg_val = arg->codegen(module, builder);
        if (!arg_val)
            return nullptr;
        args_v.push_back(arg_val);
    }

    const auto instruction_name =
        call_fn_type->getReturnType()->isVoidTy() ? "" : "indcalltmp";

    return builder->CreateCall(call_fn_type, actual_fn_ptr, args_v, instruction_name);
}

std::unique_ptr<IAstNode> AstIndirectCall::clone()
{
    ExpressionList cloned_args;
    cloned_args.reserve(this->get_args().size());
    for (const auto& arg : this->get_args())
    {
        cloned_args.push_back(arg->clone_as<IAstExpression>());
    }

    return std::make_unique<AstIndirectCall>(
        this->get_source_fragment(),
        this->get_context(),
        this->_callee->clone_as<IAstExpression>(),
        std::move(cloned_args)
    );
}

void AstIndirectCall::validate()
{
    this->get_callee()->validate();
    for (const auto& arg : this->get_args())
    {
        arg->validate();
    }
}

std::string AstIndirectCall::to_string()
{
    std::vector<std::string> arg_strs;
    arg_strs.reserve(this->get_args().size());
    for (const auto& arg : this->get_args())
    {
        arg_strs.push_back(arg->to_string());
    }

    return std::format(
        "IndirectCall(callee: {}, args: [{}])",
        this->_callee->to_string(),
        join(arg_strs, ", ")
    );
}
