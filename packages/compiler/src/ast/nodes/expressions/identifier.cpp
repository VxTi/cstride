#include "errors.h"
#include "ast/closures.h"
#include "ast/symbol_table.h"
#include "ast/nodes/expression.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

using namespace stride::ast;

void AstIdentifier::resolve_definition(const SymbolTable* symbol_table)
{
    const std::string internal_name = this->get_scoped_name();

    if (const auto var_def = symbol_table->lookup_variable(internal_name, false))
    {
        this->_definition = var_def;
        return;
    }

    // Fall back to name-based lookup, which resolves short names to their internal
    // names (e.g. `x` → `x.0` for locals with a counter suffix).
    if (const auto symbol_definition = symbol_table->lookup_symbol(internal_name))
    {
        this->_definition = symbol_definition;
        return;
    }

    // Last resort: raw name match (handles captured variables).
    if (const auto definition = symbol_table->lookup_variable(internal_name, true))
    {
        this->_definition = definition;
        return;
    }

    throw stride_error(
        ErrorType::REFERENCE_ERROR,
        std::format("Identifier '{}' not found in this scope", this->get_name()),
        this->get_source_position()
    );
}

llvm::Value* AstIdentifier::codegen_ptr(
    llvm::Module* module,
    const llvm::IRBuilderBase* builder
) const
{
    const auto definition = this->get_definition();

    const std::string internal_name = definition->get_internal_symbol_name();
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
                    return global;
                }

                throw stride_error(
                    ErrorType::REFERENCE_ERROR,
                    std::format("Identifier '{}' not found in this scope", this->get_name()),
                    this->get_source_position());
            }
        }
    }

    if (!val)
    {
        if (const auto global = module->getNamedGlobal(internal_name))
        {
            return global;
        }

        if (auto* function = module->getFunction(internal_name))
        {
            return function;
        }

        throw stride_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Identifier '{}' not found in this scope", this->get_name()),
            this->get_source_position()
        );
    }

    return val;
}

llvm::Value* AstIdentifier::codegen(
    SymbolTable* symbol_table,
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    llvm::Value* val = this->codegen_ptr(module, builder);

    if (auto* alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(val))
    {
        // Load the value from the allocated variable
        // Note: This is safe because 'val' is only found if GetInsertBlock() was not null
        return builder->CreateLoad(
            alloca->getAllocatedType(),
            alloca
        );
    }

    if (const auto* global = llvm::dyn_cast_or_null<llvm::GlobalVariable>(val))
    {
        // Only generate a Load instruction if we are inside a BasicBlock (Function context).
        if (builder->GetInsertBlock())
        {
            return builder->CreateLoad(
                global->getValueType(),
                val
            );
        }

        // If we are in Global context (initializing a global variable), we cannot generate
        // instructions. We return the GlobalVariable* itself. This allows parent nodes (like
        // MemberAccessor) to perform Constant Folding or ConstantExpr GEPs on the address.
        return val;
    }

    return val;
}

std::unique_ptr<IAstNode> AstIdentifier::clone()
{
    return std::make_unique<AstIdentifier>(this->_symbol);
}

std::string AstIdentifier::to_string()
{
    return std::format(
        "Identifier<{}({})>",
        this->get_name(),
        this->get_scoped_name()
    );
}
