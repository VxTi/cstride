#include "program.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
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
            return ProgramObject(ast::parser::parse(file));
        });

    this->_nodes = std::move(nodes);
}


void Program::execute()
{
    if (_nodes.empty())
    {
        return;
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    LLVMLinkInInterpreter();


    llvm::LLVMContext context;
    llvm::IRBuilder builder(context);
    auto module = std::make_unique<llvm::Module>("stride_module", context);

    auto* root = _nodes[0].root();
    if (auto* synthesisable = dynamic_cast<ast::ISynthesisable*>(root))
    {
        synthesisable->codegen(module.get(), context, &builder);
    }
    else
    {
        return;
    }

    if(llvm::verifyModule(*module, &llvm::errs())) {
        std::cerr << "Module verification failed!" << std::endl;
        return;
    }

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

    llvm::Function* main = engine->FindFunctionNamed("main");
    if (!main)
    {
        std::cout << "Function 'main' not found" << std::endl;
        return;
    }

    llvm::GenericValue result = engine->runFunction(main, {});
    std::cout << "Program returned: " << result.IntVal.getSExtValue() << std::endl;
}
