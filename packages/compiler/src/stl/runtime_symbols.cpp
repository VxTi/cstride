#include "ast/parsing_context.h"
#include "runtime/stride_runtime.h"
#include "runtime/symbols.h"

#include <memory>
#include <llvm/ExecutionEngine/Orc/CoreContainers.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

using namespace stride::runtime;

void stride::runtime::register_runtime_symbols(const std::shared_ptr<ast::ParsingContext>& context)
{
    const auto fragment = SourceFragment(nullptr, 0, 0);
    std::vector<std::unique_ptr<ast::IAstType>> args;
    args.push_back(std::make_unique<ast::AstPrimitiveType>(
        fragment,
        context,
        ast::PrimitiveType::STRING
    ));
    context->define_function(
        ast::Symbol(fragment, "__vprintf_internal"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::move(args),
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::INT32
            ),
            /* is_variadic = */
            true
        )
    );

    context->define_function(
        ast::Symbol(fragment, "__system_time_ns_internal"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        )
    );

    context->define_function(
        ast::Symbol(fragment, "__system_time_us_internal"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        )
    );

    context->define_function(
        ast::Symbol(fragment, "__system_time_ms_internal"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        )
    );
}

void stride::runtime::register_jit_symbols(llvm::orc::LLJIT* jit)
{
    llvm::orc::SymbolMap syms;
    auto& es = jit->getExecutionSession();
    llvm::orc::MangleAndInterner mangle(es, jit->getDataLayout());

    syms[mangle("__vprintf_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&__vprintf_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("__system_time_ns_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&__system_time_ns_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("__system_time_us_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&__system_time_us_internal),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("__system_time_ms_internal")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&__system_time_ms_internal),
        llvm::JITSymbolFlags::Exported
    );

    llvm::cantFail(jit->getMainJITDylib().define(llvm::orc::absoluteSymbols(syms)));
}
