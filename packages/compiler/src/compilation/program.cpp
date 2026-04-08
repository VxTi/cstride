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

    register_symbols(traverser);
    resolve_generics(traverser);
    resolve_types(traverser);
    initialize_forward_references(traverser, module.get(), &builder);
    generate_code(traverser, module.get(), &builder);

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

void Program::register_symbols(ast::AstNodeTraverser& traverser) const
{
    ast::ImportVisitor import_visitor;
    ast::SymbolResolver symbol_resolver;

    for (const auto& [file_name, branch] : this->_ast->get_branches())
    {
        import_visitor.set_current_file_name(file_name);
        runtime::register_runtime_symbols(branch->get_symbol_table().get());
        traverser.traverse(&import_visitor, branch.get());
        traverser.traverse(&symbol_resolver, branch.get());
    }
    import_visitor.cross_register_symbols(this->_ast.get());
}

void Program::resolve_generics(ast::AstNodeTraverser& traverser) const
{
    ast::GenericFunctionInstantiator generic_function_instantiator;

    for (const auto& branch : this->_ast->get_branches() | std::views::values)
    {
        traverser.traverse(&generic_function_instantiator, branch.get());
    }
}

void Program::resolve_types(ast::AstNodeTraverser& traverser) const
{
    ast::TypeInferenceVisitor type_visitor;

    for (const auto& branch : this->_ast->get_branches() | std::views::values)
    {
        traverser.traverse(&type_visitor, branch.get());
    }
}

void Program::initialize_forward_references(ast::AstNodeTraverser& traverser, llvm::Module* module, llvm::IRBuilder<>* builder) const
{
    ast::ForwardReferenceInitializer forward_reference_initializer(module, builder);

    for (const auto& branch : this->_ast->get_branches() | std::views::values)
    {
        traverser.traverse(&forward_reference_initializer, branch.get());
    }
}

void Program::generate_code(ast::AstNodeTraverser& traverser, llvm::Module* module, llvm::IRBuilder<>* builder) const
{
    ast::ValidationVisitor validation_visitor;

    for (const auto& branch : this->_ast->get_branches() | std::views::values)
    {
        traverser.traverse(&validation_visitor, branch.get());
        branch->get_node()->codegen(branch->get_symbol_table().get(), module, builder);
    }
}
