#include "program.h"

#include <iostream>
#include <ast/nodes/functions.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>

#include "ast/parser.h"
#include "stl/stl_functions.h"

using namespace stride;

void Program::parse_files(const std::vector<std::string>& files)
{
    this->_global_scope = std::make_shared<ast::SymbolRegistry>(ast::ScopeType::GLOBAL);
    this->_files = files;

    stl::predefine_symbols(this->get_global_scope());

    std::vector<std::unique_ptr<ast::AstBlock>> ast_nodes;

    for (const auto& file : files)
    {
        auto root_node = ast::parser::parse_file(*this, file);

        if (!root_node)
            continue;

        ast_nodes.push_back(std::move(root_node));
    }

    if (ast_nodes.empty())
    {
        std::cout << "No valid stride files found" << std::endl;
        exit(0);
    }

    auto first_node = std::move(ast_nodes.front());

    // Append all other nodes into first
    for (auto it = ast_nodes.begin() + 1; it != ast_nodes.end(); ++it)
    {
        first_node->aggregate_block(it->get());
    }

    this->_root_node = std::move(first_node);
}

void Program::print_ast_nodes() const
{
    for (const auto& node : this->_root_node->children())
    {
        std::cout << node->to_string() << std::endl;
    }
}

void Program::optimize_ast_nodes()
{
    std::vector<std::unique_ptr<ast::IAstNode>> new_children;
    for (auto& child_ptr : this->_root_node->children())
    {
        ast::IAstNode* child = child_ptr.get();

        if (auto* reducible = dynamic_cast<ast::IReducible*>(child))
        {
            if (auto* reduced = reducible->reduce())
            {
                // Note: Assuming reduce() returns a new raw pointer or
                // you manage ownership correctly in your AST implementation.
                new_children.push_back(std::unique_ptr<ast::IAstNode>(reduced));
                continue;
            }
        }
        // If not reducible or reduction failed, move the original node
        // This requires changing the loop to take ownership or clone
        new_children.push_back(std::unique_ptr<ast::IAstNode>(child));
    }

    this->_root_node = std::make_unique<ast::AstBlock>(
        this->_root_node->source,
        this->_root_node->source_offset,
        this->get_global_scope(),
        std::move(new_children)
    );
}

void Program::validate_ast_nodes() const
{
    for (const auto& child : this->_root_node->children())
    {
        child->validate();
    }
}

void Program::resolve_forward_references(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
) const
{
    this->_root_node->resolve_forward_references(this->get_global_scope(), module, context, builder);
}

void Program::generate_llvm_ir(
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
) const
{
    if (auto* synthesisable = dynamic_cast<ast::ISynthesisable*>(this->_root_node.get()))
    {
        if (const auto entry = synthesisable->codegen(this->get_global_scope(), module, context, builder); !
            entry)
        {
            throw std::runtime_error(
                "Failed to build executable for file " + this->_root_node->source->path);
        }
    }
}

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
        llvm::logAllUnhandledErrors(main_symbol_or_err.takeError(), llvm::errs(), "JIT lookup error: ");
        exit(1);
    }

    return main_symbol_or_err;
}

int Program::compile_jit() const
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto thread_safe_context = std::make_unique<llvm::orc::ThreadSafeContext>(
        std::make_unique<llvm::LLVMContext>()
    );

    auto* context = thread_safe_context->withContextDo([](llvm::LLVMContext* C)
    {
        return C;
    });

    auto jtmbOrErr = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!jtmbOrErr)
    {
        llvm::logAllUnhandledErrors(jtmbOrErr.takeError(), llvm::errs(), "JITTargetMachineBuilder error: ");
        return 1;
    }
    auto jtmb = std::move(*jtmbOrErr);


    // We explicitly create the TargetMachine to use it for both the JIT and the Optimizer
    auto target_machine = llvm::cantFail(jtmb.createTargetMachine());

    // Build the JIT using the existing TargetMachineBuilder
    auto jit = llvm::cantFail(llvm::orc::LLJITBuilder()
                             .setJITTargetMachineBuilder(std::move(jtmb))
                             .create());

    jit->getMainJITDylib().addGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            jit->getDataLayout().getGlobalPrefix()))
    );

    const auto triple = llvm::Triple(jit->getTargetTriple().getTriple());
    auto module = std::make_unique<llvm::Module>("stride_jit_module", *context);

    module->setDataLayout(jit->getDataLayout());
    module->setTargetTriple(triple);

    llvm::IRBuilder<> builder(*context);
    stl::llvm_jit_define_functions(jit.get());
    stl::llvm_insert_function_definitions(module.get(), *context);

    this->validate_ast_nodes();
    this->resolve_forward_references(module.get(), *context, &builder);
    this->generate_llvm_ir(module.get(), *context, &builder);

    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        throw std::runtime_error("LLVM IR verification failed");
    }

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // Use the target_machine we created earlier
    llvm::PassBuilder PB(target_machine.get());

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    MPM.run(*module, MAM);

    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), std::move(*thread_safe_context));
    llvm::cantFail(jit->addIRModule(std::move(tsm)));

    const auto main_fn_executor = locate_main_fn(jit.get());

    auto main_fn = main_fn_executor->toPtr<int (*)()>();
    return main_fn();
}
