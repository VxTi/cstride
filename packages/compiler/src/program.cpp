#include "program.h"

#include <iostream>
#include <ast/symbols.h>
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

void Program::parse_files(std::vector<std::string> files)
{
    this->_global_scope = std::make_shared<ast::ParsingContext>();
    this->_files = std::move(files);

    stl::predefine_symbols(this->get_global_scope());

    std::vector<std::unique_ptr<ast::AstBlock>> ast_nodes;

    for (const auto& file : this->_files)
    {
        auto parsed = ast::parser::parse_file(*this, file);
        if (!parsed)
        {
            std::cout << "Failed to parse file: " << file << std::endl;
            exit(1);
        }
        ast_nodes.push_back(std::move(parsed));
    }

    if (this->_files.empty())
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
                std::cout << "Optimized node: " << child->to_string() << " to " << reduced->to_string() << std::endl;
                continue;
            }
        }
        // If not reducible or reduction failed, move the original node
        // This requires changing the loop to take ownership or clone
        new_children.push_back(std::unique_ptr<ast::IAstNode>(child));
    }

    this->_root_node = std::make_unique<ast::AstBlock>(
        this->_root_node->get_source(),
        this->_root_node->get_source_position(),
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

void Program::resolve_forward_references(llvm::Module* module, llvm::IRBuilder<>* builder) const
{
    this->_root_node->resolve_forward_references(this->get_global_scope(), module, builder);
}

void Program::generate_llvm_ir(
    llvm::Module* module,
    llvm::IRBuilder<>* builder
) const
{
    if (auto* synthesisable = dynamic_cast<ast::ISynthesisable*>(this->_root_node.get()))
    {
        if (const auto entry = synthesisable->codegen(this->get_global_scope(), module, builder);
            entry == nullptr)
        {
            throw std::runtime_error(
                std::format("Failed to build executable for file '{}'", this->_root_node->get_source()->path)
            );
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

int Program::compile_jit(const cli::CompilationOptions& options) const
{
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    setvbuf(stdout, nullptr, _IONBF, 0);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto tsc = llvm::orc::ThreadSafeContext(std::make_unique<llvm::LLVMContext>());

    auto* context = tsc.withContextDo([](llvm::LLVMContext* ctx)
    {
        return ctx;
    });

    auto jit_target_machine_builder = llvm::orc::JITTargetMachineBuilder::detectHost();

    if (!jit_target_machine_builder)
    {
        llvm::logAllUnhandledErrors(jit_target_machine_builder.takeError(), llvm::errs(),
                                    "JITTargetMachineBuilder error: ");
        return 1;
    }
    auto jtmb = std::move(*jit_target_machine_builder);


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
    stl::llvm_insert_function_definitions(module.get());

    this->validate_ast_nodes();
    this->resolve_forward_references(module.get(), &builder);
    this->generate_llvm_ir(module.get(), &builder);


    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        module->print(llvm::errs(), nullptr);
        throw std::runtime_error("LLVM IR verification failed");
    }

    if (options.debug_mode)
    {
        std::cout << "LLVM IR Pre-optimizations:" << std::endl;
        module->print(llvm::errs(), nullptr);
    }

    llvm::LoopAnalysisManager loop_analysis_manager;
    llvm::FunctionAnalysisManager function_analysis_manager;
    llvm::CGSCCAnalysisManager cgscc_analysis_manager;
    llvm::ModuleAnalysisManager module_analysis_manager;

    // Use the target_machine we created earlier
    llvm::PassBuilder pass_builder(target_machine.get());

    pass_builder.registerModuleAnalyses(module_analysis_manager);
    pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
    pass_builder.registerFunctionAnalyses(function_analysis_manager);
    pass_builder.registerLoopAnalyses(loop_analysis_manager);
    pass_builder.crossRegisterProxies(
        loop_analysis_manager,
        function_analysis_manager,
        cgscc_analysis_manager,
        module_analysis_manager
    );

    llvm::ModulePassManager module_pass_manager = pass_builder.buildPerModuleDefaultPipeline(
        llvm::OptimizationLevel::O3);
    module_pass_manager.run(*module, module_analysis_manager);

    llvm::orc::ThreadSafeModule thread_safe_module(
        std::move(module),
        std::move(tsc)
    );
    llvm::cantFail(jit->addIRModule(std::move(thread_safe_module)));

    if (auto err = jit->initialize(jit->getMainJITDylib()))
    {
        llvm::logAllUnhandledErrors(std::move(err), llvm::errs(), "JIT initialization error: ");
        return 1;
    }

    const auto main_fn_executor = locate_main_fn(jit.get());

    if (!main_fn_executor.get())
    {
        throw std::runtime_error("Main function not found");
    }
    fflush(stdout);

    auto main_fn = main_fn_executor->toPtr<int (*)()>();
    int result = main_fn();

    if (auto err = jit->deinitialize(jit->getMainJITDylib()))
    {
        llvm::logAllUnhandledErrors(std::move(err), llvm::errs(), "JIT deinitialization error: ");
    }

    return result;
}
