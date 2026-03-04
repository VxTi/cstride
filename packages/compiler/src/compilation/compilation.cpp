#include "program.h"

#include <iostream>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>

using namespace stride;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

int Program::compile(const cli::CompilationOptions& options) const
{
    // Initialize LLVM targets
#if CSTRIDE_ALL_TARGETS
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
#else
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
#endif

    auto target_triple_str = options.target_triple.empty()
        ? llvm::sys::getDefaultTargetTriple()
        : options.target_triple;
    llvm::Triple target_triple(target_triple_str);
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple_str, error);

    if (!target)
    {
        llvm::errs() << error;
        return 1;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto rm = std::optional<llvm::Reloc::Model>();
    auto target_machine =
        target->createTargetMachine(target_triple, cpu, features, opt, rm);

    llvm::LLVMContext context;
    auto module = prepare_module(context, options, target_machine);

    auto filename = options.program_name.empty() ? "output.o" : options.program_name + ".o";
    if (!options.output_path.empty())
    {
        filename = options.output_path + "/" + filename;
    }

    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec)
    {
        llvm::errs() << "Could not open file: " << ec.message();
        return 1;
    }

    // Emit object file
    llvm::legacy::PassManager pass;

    if (auto file_type = llvm::CodeGenFileType::ObjectFile;
        target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type))
    {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(*module);
    dest.flush();

    std::cout << "Object file generated: " << filename << std::endl;

    // Link object file to create executable
    // This is a simplified linking step. In a real-world scenario, you might want to identify the correct linker or use clang/gcc to link.
    // Assuming clang is available for linking.

    std::string output_binary = options.program_name.empty() ? "executable" : options.program_name;
    if (!options.output_path.empty())
    {
        output_binary = std::format("{}/{}", options.output_path, output_binary);
    }

    std::string runtime_path = TOSTRING(STRIDE_RUNTIME_LIB_PATH);
    std::vector<std::string> files = { runtime_path, filename};

    // Dead-code elimination and stripping differ by platform.
    //
    // macOS (Apple ld):
    //   -Wl,-dead_strip   removes unreferenced sections
    //   -s is obsolete;   use a post-link `strip -S -x` instead
    //     -S  strips debug info (removes embedded source paths)
    //     -x  strips local (non-global) symbols
    //
    // Linux (GNU ld / lld):
    //   -Wl,--gc-sections removes unreferenced sections
    //     (requires -ffunction-sections/-fdata-sections on the objects,
    //      already set for stride_runtime in CMakeLists.txt)
    //   -Wl,--strip-all   strips all symbols in one linker pass
    const bool is_darwin = target_triple.isOSDarwin();
    std::string dead_strip_flag = is_darwin ? "-Wl,-dead_strip" : "-Wl,--gc-sections";
    std::string strip_flag      = is_darwin ? ""                : "-Wl,--strip-all";

    std::string linker_command = std::format("clang++ {} -o {} {} {}",
        join(files, " "), output_binary, dead_strip_flag, strip_flag);

    if (options.debug_mode)
    {
        std::cout << "Linking command: " << linker_command << std::endl;
    }

    if (int link_result = system(linker_command.c_str());
        link_result != 0)
    {
        std::cerr << "Linking failed." << std::endl;
        return link_result;
    }

    // On macOS, -s is obsolete in Apple ld. Run strip(1) after linking instead:
    //   -S  removes debug info (DWARF / stabs) — eliminates embedded build paths
    //   -x  removes local (non-exported) symbols — reduces symbol table size
    if (is_darwin)
    {
        std::string strip_command = std::format("strip -S -x {}", output_binary);
        if (options.debug_mode)
            std::cout << "Strip command: " << strip_command << std::endl;
        if (int strip_result = system(strip_command.c_str());
            strip_result != 0)
        {
            std::cerr << "strip failed." << std::endl;
            return strip_result;
        }
    }

    std::cout << "Executable created: " << output_binary << std::endl;

    // Clean up object file if desired? or keep it?
    // Usually compilers keep .o files if requested, otherwise maybe delete?
    // For now we keep it as it was generated.

    return 0;
}
