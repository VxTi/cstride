#include "program.h"

#include "ast/ast.h"
#include "ast/visitor.h"
#include "../../include/ast/traversal.h"
#include "runtime/symbols.h"

#include <iostream>
#include <ranges>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>

using namespace stride;

Program Program::from_sources(const std::vector<std::string>& files)
{
    if (files.empty())
    {
        std::cout << "No valid stride files found" << std::endl;
        exit(0);
    }

    auto ast = ast::Ast::parse_files(files);

    return Program(std::move(ast));
}

std::unique_ptr<llvm::Module> Program::prepare_module(
    llvm::LLVMContext& context,
    const cli::CompilationOptions& options,
    llvm::TargetMachine* target_machine) const
{
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    auto module = std::make_unique<llvm::Module>("stride_module", context);
    module->setDataLayout(target_machine->createDataLayout());
    module->setTargetTriple(target_machine->getTargetTriple());

    llvm::IRBuilder<> builder(context);

    ast::AstNodeTraverser traverser;

    ast::TypeInferenceVisitor type_visitor;
    ast::ValidationVisitor validation_visitor;
    ast::SymbolResolver symbol_resolver;
    ast::ForwardReferenceInitializer forward_reference_initializer(module.get(), &builder);
    ast::GenericFunctionInstantiator generic_function_instantiator;
    ast::ImportVisitor import_visitor;

    //
    // First step - Cross-file symbol registration (imports and function signatures)
    //
    for (const auto& [file_name, branch] : this->_ast->get_branches())
    {
        printf("--- Traversing %s ---\n", file_name.c_str());
        // Resolve imports and populate local registry - Used for cross registration step
        import_visitor.set_current_file_name(file_name);

        // Populate symbol table with stride runtime symbols
        // These are externally available functions that are linked after codegen
        runtime::register_runtime_symbols(branch->get_symbol_table().get());
        traverser.traverse(&import_visitor, branch.get());
        traverser.traverse(&symbol_resolver, branch.get());
    }
    import_visitor.cross_register_symbols(this->_ast.get());

    //
    // Generic template resolution - Instantiates functions that have generic arguments
    //
    for (const auto& [file_name, branch] : this->_ast->get_branches())
    {
        printf("--- Generic instantiation | Traversing %s ---\n", file_name.c_str());
        traverser.traverse(&generic_function_instantiator, branch.get());
    }

    //
    // Third step - Type resolution
    //
    for (const auto& [file_name, branch] : this->_ast->get_branches())
    {
        printf("--- Type resolution | Traversing %s ---\n", file_name.c_str());
        traverser.traverse(&type_visitor, branch.get());
    }

    for (const auto& branch : this->_ast->get_branches() | std::views::values)
    {
        forward_reference_initializer.accept(branch->get_symbol_table().get(), branch->get_node());
        // Resolving forward references - Ensures symbols certain symbols are available before implementation
    }

    /// --- Final step - LLVM IR validation and code generation
    for (const auto& branch : this->_ast->get_branches() | std::views::values)
    {
        traverser.traverse(&validation_visitor, branch.get());
        branch->get_node()->codegen(branch->get_symbol_table().get(), module.get(), &builder);
    }

    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        module->print(llvm::errs(), nullptr);
        throw std::runtime_error("LLVM IR verification failed");
    }

    if (options.debug_mode)
    {
        module->print(llvm::errs(), nullptr);
    }

    // optimizing
    llvm::LoopAnalysisManager loop_analysis_manager;
    llvm::FunctionAnalysisManager function_analysis_manager;
    llvm::CGSCCAnalysisManager cgscc_analysis_manager;
    llvm::ModuleAnalysisManager module_analysis_manager;

    llvm::PassBuilder pass_builder(target_machine);

    pass_builder.registerModuleAnalyses(module_analysis_manager);
    pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
    pass_builder.registerFunctionAnalyses(function_analysis_manager);
    pass_builder.registerLoopAnalyses(loop_analysis_manager);
    pass_builder.crossRegisterProxies(
        loop_analysis_manager,
        function_analysis_manager,
        cgscc_analysis_manager,
        module_analysis_manager);

    llvm::ModulePassManager module_pass_manager =
        pass_builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);
    module_pass_manager.run(*module, module_analysis_manager);

    return module;
}
