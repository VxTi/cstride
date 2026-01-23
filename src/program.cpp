#include "program.h"

#include <iostream>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>

#include "ast/parser.h"

using namespace stride;

void Program::parse_files(const std::vector<std::string>& files)
{
    this->_global_scope = std::make_shared<ast::Scope>(ast::ScopeType::GLOBAL);
    this->_files = files;

    for (const auto& file : files)
    {
        auto root_node = ast::parser::parse_file(*this, file);

        if (const auto reducible = dynamic_cast<ast::IReducible*>(root_node.get());
            reducible && reducible->is_reducible())
        {
            // It's possible the node can be completely reduced,
            // so we'll have to be careful here.
            if (auto* reduced_raw = reducible->reduce())
            {
                std::unique_ptr<ast::IAstNode> reduced_node(reduced_raw);

                this->_program_objects.push_back(
                    std::make_unique<ProgramObject>(std::move(reduced_node))
                );
            }
        }
        else
        {
            this->_program_objects.push_back(
                std::make_unique<ProgramObject>(std::move(root_node))
            );
        }
    }
}

void Program::print_ast_nodes() const
{
    for (const auto& node : _program_objects)
    {
        std::cout << node->get_root_ast_node()->source->path;
        std::cout << node->get_root_ast_node()->to_string() << std::endl;
    }
}

void Program::execute(int argc, char* argv[]) const
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    if (_program_objects.empty())
    {
        return;
    }
    // For debugging purposes, comment this out to see the generated AST nodes
    this->print_ast_nodes();

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMLinkInInterpreter();
    LLVMLinkInMCJIT();


    llvm::LLVMContext context;
    llvm::IRBuilder builder(context);

    auto module = std::make_unique<llvm::Module>("stride_module", context);
    const std::string target_triple_str = llvm::sys::getDefaultTargetTriple();
    const auto triple = llvm::Triple(target_triple_str);

    std::string message;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(
        target_triple_str, message
    );
    if (!target) throw std::runtime_error("Couldn't find target.");

    llvm::TargetMachine* const machine = target->createTargetMachine(
        triple,
        "generic",
        "",
        llvm::TargetOptions(),
        std::optional<llvm::Reloc::Model>()
    );

    module->setDataLayout(machine->createDataLayout());
    module->setTargetTriple(triple);

    // First we'll define symbols for all nodes
    // This allows for function calls even if they're defined below the callee
    for (const auto& node : _program_objects)
    {
        if (auto* synthesisable = dynamic_cast<ast::ISynthesisable*>(node->get_root_ast_node()))
        {
            synthesisable->define_symbols(module.get(), context, &builder);
        }
    }

    for (const auto& node : _program_objects)
    {
        if (auto* synthesisable = dynamic_cast<ast::ISynthesisable*>(node->get_root_ast_node()))
        {
            if (const auto entry = synthesisable->codegen(TODO, module.get(), context, &builder); !entry)
            {
                throw std::runtime_error(
                    "Failed to build executable for file " + node->get_root_ast_node()->source->path
                );
            }
        }
    }

    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        std::cerr << "Module verification failed!" << std::endl;
        module->print(llvm::errs(), nullptr);
        return;
    }


    // Will print the LLVM IR
    module->print(llvm::errs(), nullptr);

    std::string error;
    llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::move(module))
                                   .setErrorStr(&error)
                                   .setEngineKind(llvm::EngineKind::JIT)
                                   .create();

    if (!engine)
    {
        std::cout << "Failed to create execution engine: " << error << std::endl;
        return;
    }

    engine->finalizeObject();
    const uint64_t func_addr = engine->getFunctionAddress(MAIN_FN_NAME);

    if (func_addr == 0)
    {
        std::cout << "Function '" << MAIN_FN_NAME << "' not found" << std::endl;
        return;
    }

    typedef int (*MainFunc)(int, char**);
    auto main_func = reinterpret_cast<MainFunc>(func_addr);

    const int status_code = main_func(argc, argv);

    // Ensure we exit this process with the same status code as the interpreted process
    exit(status_code);
}
