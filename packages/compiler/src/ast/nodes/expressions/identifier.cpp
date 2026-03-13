#include "errors.h"
#include "ast/closures.h"
#include "ast/parsing_context.h"
#include "ast/nodes/expression.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

using namespace stride::ast;

std::optional<const definition::IDefinition*> AstIdentifier::get_definition() const
{
    const std::string internal_name = this->get_scoped_name();

    if (const auto var_def = this->get_context()->lookup_variable(internal_name, false))
    {
        return var_def;
    }

    // Fall back to name-based lookup, which resolves short names to their internal
    // names (e.g. `x` → `x.0` for locals with a counter suffix).
    if (const auto symbol_definition = this->get_context()->lookup_symbol(internal_name))
    {
        return symbol_definition;
    }

    // Last resort: raw name match (handles captured variables).
    if (const auto definition = this->get_context()->lookup_variable(internal_name, true))
    {
        return definition;
    }

    return std::nullopt;
}

llvm::Value* AstIdentifier::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    const auto definition = this->get_definition();

    if (!definition.has_value())
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Identifier '{}' not found in this scope", this->get_name()),
            this->get_source_fragment()
        );
    }

    const std::string internal_name = definition.value()->get_internal_symbol_name();
    llvm::Value* val = nullptr;

    if (const auto block = builder->GetInsertBlock())
    {
        if (llvm::Function* function = block->getParent())
        {
            val = closures::lookup_variable_or_capture(function, internal_name);

            // If not found by exact name, try base name lookup
            // This handles cases where variables have numeric suffixes (e.g., "x.0")
            if (!val)
            {
                val = closures::lookup_variable_by_base_name(function, this->get_name());
            }

            if (!val)
            {
                // Check if the identifier refers to a function defined in the module
                if (auto* fn = module->getFunction(internal_name))
                {
                    return fn;
                }

                if (auto* global = module->getNamedGlobal(internal_name))
                {
                    return builder->CreateLoad(
                        global->getValueType(),
                        global,
                        internal_name
                    );
                }

                if (auto* arg = llvm::dyn_cast_or_null<llvm::Argument>(val))
                {
                    return arg;
                }

                throw parsing_error(
                    ErrorType::REFERENCE_ERROR,
                    std::format("Identifier '{}' not found in this scope", this->get_name()),
                    this->get_source_fragment());
            }
        }
    }

    // Check if it's a function argument
    if (auto* arg = llvm::dyn_cast_or_null<llvm::Argument>(val))
    {
        return arg;
    }

    if (auto* alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(val))
    {
        // Load the value from the allocated variable
        // Note: This is safe because 'val' is only found if GetInsertBlock() was not null
        return builder->CreateLoad(
            alloca->getAllocatedType(),
            alloca,
            internal_name
        );
    }

    if (const auto global = module->getNamedGlobal(internal_name))
    {
        // Only generate a Load instruction if we are inside a BasicBlock (Function context).
        if (builder->GetInsertBlock())
        {
            return builder->CreateLoad(
                global->getValueType(),
                global,
                internal_name
            );
        }

        // If we are in Global context (initializing a global variable), we cannot generate
        // instructions. We return the GlobalVariable* itself. This allows parent nodes (like
        // MemberAccessor) to perform Constant Folding or ConstantExpr GEPs on the address.
        return global;
    }

    if (auto* function = module->getFunction(internal_name))
    {
        return function;
    }

    throw parsing_error(
        ErrorType::REFERENCE_ERROR,
        std::format("Identifier '{}' not found in this scope", this->get_name()),
        this->get_source_fragment()
    );
}

std::unique_ptr<IAstNode> AstIdentifier::clone()
{
    return std::make_unique<AstIdentifier>(
        this->get_context(),
        this->_symbol
    );
}

std::string AstIdentifier::to_string()
{
    return std::format(
        "Identifier<{}({})>",
        this->get_name(),
        this->get_scoped_name()
    );
}
