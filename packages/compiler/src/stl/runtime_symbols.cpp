#include "ast/flags.h"
#include "ast/symbol_table.h"
#include "runtime/stride_runtime.h"
#include "runtime/symbols.h"

#include <memory>
#include <llvm/ExecutionEngine/Orc/CoreContainers.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

using namespace stride::runtime;

void stride::runtime::register_runtime_symbols(ast::SymbolTable* symbol_table)
{
    const auto source = std::make_shared<SourceFile>("unknown", "");
    const auto position = SourcePosition(source, 0, 0);
    std::vector<std::unique_ptr<ast::IAstType>> args;
    args.push_back(std::make_unique<ast::AstPrimitiveType>(
        position,
        ast::PrimitiveType::STRING
    ));
    symbol_table->define_function(
        ast::Symbol(position, "_printf_internal"),
        std::make_unique<ast::AstFunctionType>(
            position,
            std::move(args),
            std::make_unique<ast::AstPrimitiveType>(
                position,
                ast::PrimitiveType::INT32
            )
        ),
        ast::VisibilityModifier::PUBLIC,
        SRFLAG_FN_TYPE_VARIADIC
    );

    symbol_table->define_function(
        ast::Symbol(position, "_system_time_ns_internal"),
        std::make_unique<ast::AstFunctionType>(
            position,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                position,
                ast::PrimitiveType::UINT64
            )
        ),
        ast::VisibilityModifier::PUBLIC
    );

    symbol_table->define_function(
        ast::Symbol(position, "_system_time_us_internal"),
        std::make_unique<ast::AstFunctionType>(
            position,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                position,
                ast::PrimitiveType::UINT64
            )
        ),
        ast::VisibilityModifier::PUBLIC
    );

    symbol_table->define_function(
        ast::Symbol(position, "_system_time_ms_internal"),
        std::make_unique<ast::AstFunctionType>(
            position,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                position,
                ast::PrimitiveType::UINT64
            )
        ),
        ast::VisibilityModifier::PUBLIC
    );

    std::vector<std::unique_ptr<ast::IAstType>> read_in_params;
    read_in_params.push_back(std::make_unique<ast::AstPrimitiveType>(
        position,
        ast::PrimitiveType::INT32
    ));
    symbol_table->define_function(
        ast::Symbol(position, "_read_in_internal"),
        std::make_unique<ast::AstFunctionType>(
            position,
            std::move(read_in_params),
            std::make_unique<ast::AstPrimitiveType>(
                position,
                ast::PrimitiveType::STRING
            )
        ),
        ast::VisibilityModifier::PUBLIC
    );
}

void stride::runtime::register_jit_symbols(llvm::orc::LLJIT* jit)
{
    llvm::orc::SymbolMap syms;
    auto& es = jit->getExecutionSession();
    llvm::orc::MangleAndInterner mangle(es, jit->getDataLayout());

    syms[mangle("_printf_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&_printf_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("_system_time_ns_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&_system_time_ns_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("_system_time_us_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&_system_time_us_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("_system_time_ms_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&_system_time_ms_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("_read_in_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&_read_in_internal),
        llvm::JITSymbolFlags::Exported
    );

    llvm::cantFail(jit->getMainJITDylib().define(llvm::orc::absoluteSymbols(syms)));
}
