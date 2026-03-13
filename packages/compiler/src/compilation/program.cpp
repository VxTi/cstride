#include "program.h"

#include "ast/ast.h"
#include "ast/visitor.h"
#include "ast/nodes/traversal.h"
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
    ast::ExpressionVisitor type_visitor;
    ast::FunctionVisitor function_visitor;
    ast::ImportVisitor import_visitor;

    for (const auto& [file_name, node] : this->_ast->get_files())
    {
        import_visitor.set_current_file_name(file_name);
        traverser.visit_block(&import_visitor, node.get());

        traverser.visit_block(&function_visitor, node.get());
    }
    import_visitor.cross_register_symbols(this->_ast.get());

    for (const auto& node : this->_ast->get_files() | std::views::values)
    {
        runtime::register_runtime_symbols(node->get_context());
        traverser.visit_block(&type_visitor, node.get());

        node->validate();
        node->resolve_forward_references(
            module.get(),
            &builder
        );
        node->codegen(module.get(), &builder);
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
