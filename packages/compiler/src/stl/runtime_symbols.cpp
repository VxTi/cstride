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
        ast::Symbol(fragment, "printf"),
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
        ),
        0
    );

    context->define_function(
        ast::Symbol(fragment, "system_time_ns"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        ),
        0
    );

    context->define_function(
        ast::Symbol(fragment, "system_time_us"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        ),
        0
    );

    context->define_function(
        ast::Symbol(fragment, "system_time_ms"),
        std::make_unique<ast::AstFunctionType>(
            fragment,
            context,
            std::vector<std::unique_ptr<ast::IAstType>>{},
            std::make_unique<ast::AstPrimitiveType>(
                fragment,
                context,
                ast::PrimitiveType::UINT64
            )
        ),
        0
    );
}

void stride::runtime::register_jit_symbols(llvm::orc::LLJIT* jit)
{
    llvm::orc::SymbolMap syms;
    auto& es = jit->getExecutionSession();
    llvm::orc::MangleAndInterner mangle(es, jit->getDataLayout());

    syms[mangle("printf")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&stride_printf),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("vprintf")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&vprintf),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("system_time_ns")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&system_time_ns),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("system_time_us")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&system_time_us),
        llvm::JITSymbolFlags::Exported
    );
    syms[mangle("system_time_ms")] = llvm::orc::ExecutorSymbolDef(
        llvm::orc::ExecutorAddr::fromPtr(&system_time_ms),
        llvm::JITSymbolFlags::Exported
    );

    llvm::cantFail(jit->getMainJITDylib().define(llvm::orc::absoluteSymbols(syms)));
}
