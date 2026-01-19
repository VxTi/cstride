#include "program.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <iostream>

#include "ast/parser.h"

using namespace stride;

Program::Program(std::vector<std::string> files)
{
    std::vector<ProgramObject> nodes;

    // Transform provided files into Program Objects (IR file representation)
    std::ranges::transform(
        files, std::back_inserter(nodes),
        [](const std::string& file) -> ProgramObject
        {
            auto root_node = ast::parser::parse(file);

            if (const auto reducible = dynamic_cast<ast::IReducible*>(root_node.get());
                reducible && reducible->is_reducible())
            {
                return ProgramObject(reducible->reduce());
            }

            return ProgramObject(std::move(root_node));
        });

    this->_nodes = std::move(nodes);
}


void Program::execute(int argc, char* argv[]) const
{
    if (_nodes.empty())
    {
        return;
    }
    // For debugging purposes, comment this out to see the generated AST nodes
    // std::cout << _nodes[0].root()->to_string() << std::endl;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMLinkInInterpreter();


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


    auto* root = _nodes[0].root();
    if (auto* synthesisable = dynamic_cast<ast::ISynthesisable*>(root))
    {
        if (const auto entry = synthesisable->codegen(module.get(), context, &builder); !entry)
        {
            std::cerr << "Failed to codegen entry point" << std::endl;
            return;
        }
    }
    else
    {
        return;
    }

    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        std::cerr << "Module verification failed!" << std::endl;
        module->print(llvm::errs(), nullptr);
        return;
    }


    // Will print the LLVM IR
    // module->print(llvm::errs(), nullptr);

    std::string error;
    llvm::ExecutionEngine* engine = llvm::EngineBuilder(std::move(module))
                                   .setErrorStr(&error)
                                   .setEngineKind(llvm::EngineKind::Interpreter)
                                   .create();

    if (!engine)
    {
        std::cout << "Failed to create execution engine: " << error << std::endl;
        return;
    }

    llvm::Function* main = engine->FindFunctionNamed(MAIN_FN_NAME);
    if (!main)
    {
        std::cout << "Function '" << MAIN_FN_NAME << "' not found" << std::endl;
        return;
    }


    std::vector<llvm::GenericValue> ArgValues;

    llvm::GenericValue ArgcValue;
    ArgcValue.IntVal = llvm::APInt(32, argc);
    ArgValues.push_back(ArgcValue);

    llvm::GenericValue ArgvValue;
    ArgvValue.PointerVal = argv;
    ArgValues.push_back(ArgvValue);

    llvm::GenericValue result = engine->runFunction(main, ArgValues);

    const auto status_code = static_cast<int>(result.IntVal.getZExtValue());

    std::cout << "\nProcess exited with status code " << status_code << std::endl;
}
