#include "program.h"
#include "stl/stl_functions.h"

#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/TargetSelect.h>

using namespace stride;

llvm::Expected<llvm::orc::ExecutorAddr> locate_main_fn(llvm::orc::LLJIT* jit)
{
    // First check whether we can find the unmangled version of the main function
    if (auto resolved_symbol = jit->lookup(MAIN_FN_NAME))
    {
        return resolved_symbol;
    }

    // Otherwise we'll have to find the mangled name
    const auto mangled_name = jit->mangleAndIntern(MAIN_FN_NAME);
    auto main_symbol_or_err = jit->lookup(*mangled_name);
    if (!main_symbol_or_err)
    {
        llvm::logAllUnhandledErrors(
            main_symbol_or_err.takeError(),
            llvm::errs(),
            "JIT lookup error: ");
        exit(1);
    }

    return main_symbol_or_err;
}

int Program::compile_jit(const cli::CompilationOptions& options) const
{
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    setvbuf(stdout, nullptr, _IONBF, 0);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto tsc = llvm::orc::ThreadSafeContext(
        std::make_unique<llvm::LLVMContext>());

    auto* context = tsc.withContextDo([](llvm::LLVMContext* ctx)
    {
        return ctx;
    });

    auto jit_target_machine_builder =
        llvm::orc::JITTargetMachineBuilder::detectHost();

    if (!jit_target_machine_builder)
    {
        llvm::logAllUnhandledErrors(
            jit_target_machine_builder.takeError(),
            llvm::errs(),
            "JITTargetMachineBuilder error: "
        );
        return 1;
    }
    auto jtmb = std::move(*jit_target_machine_builder);


    // We explicitly create the TargetMachine to use it for both the JIT and the Optimizer
    const auto target_machine = llvm::cantFail(jtmb.createTargetMachine());

    // Register our runtime symbols manually to ensure they are available
    // Build the JIT using the existing TargetMachineBuilder
    const auto jit = llvm::cantFail(
        llvm::orc::LLJITBuilder()
       .setJITTargetMachineBuilder(std::move(jtmb))
       .create()
    );
    stl::register_runtime_symbols(jit.get());
    auto module = prepare_module(*context, options, target_machine.get());

    jit->getMainJITDylib().addGenerator(
        llvm::cantFail(
            llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                jit->getDataLayout().getGlobalPrefix()))
    );

    llvm::orc::ThreadSafeModule thread_safe_module(
        std::move(module),
        std::move(tsc));
    llvm::cantFail(jit->addIRModule(std::move(thread_safe_module)));

    if (auto err = jit->initialize(jit->getMainJITDylib()))
    {
        llvm::logAllUnhandledErrors(
            std::move(err),
            llvm::errs(),
            "JIT initialization error: ");
        return 1;
    }

    const auto main_fn_executor = locate_main_fn(jit.get());

    if (!main_fn_executor.get())
    {
        throw std::runtime_error("Main function not found");
    }
    fflush(stdout);

    const auto main_fn = main_fn_executor->toPtr<int (*)()>();
    const int result = main_fn();

    if (auto err = jit->deinitialize(jit->getMainJITDylib()))
    {
        llvm::logAllUnhandledErrors(
            std::move(err),
            llvm::errs(),
            "JIT deinitialization error: ");
    }

    return result;
}
