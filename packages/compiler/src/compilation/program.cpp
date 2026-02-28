#include "program.h"

#include "ast/parser.h"
#include "stl/stl_functions.h"

#include <iostream>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>

using namespace stride;

void Program::parse_files(std::vector<std::string> files)
{
    this->_global_scope = std::make_shared<ast::ParsingContext>();
    this->_files = std::move(files);

    stl::register_internal_symbols(this->get_global_context());

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
                std::cout << "Optimized node: " << child->to_string() << " to "
                    << reduced->to_string() << std::endl;
                continue;
            }
        }
        // If not reducible or reduction failed, move the original node
        // This requires changing the loop to take ownership or clone
        new_children.push_back(std::unique_ptr<ast::IAstNode>(child));
    }

    this->_root_node = std::make_unique<ast::AstBlock>(
        this->_root_node->get_source_fragment(),
        this->get_global_context(),
        std::move(new_children));
}

void Program::validate_ast_nodes() const
{
    for (const auto& child : this->_root_node->children())
    {
        child->validate();
    }
}

void Program::resolve_forward_references(llvm::Module* module, llvm::IRBuilderBase* builder) const
{
    this->_root_node->resolve_forward_references(
        this->get_global_context().get(),
        module,
        builder
    );
}

void Program::generate_llvm_ir(llvm::Module* module, llvm::IRBuilderBase* builder) const
{
    if (this->_root_node->codegen(module, builder) == nullptr)
    {
        throw std::runtime_error(
            std::format(
                "Failed to build executable for file '{}'",
                this->_root_node->get_source()->path
            )
        );
    }
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

    llvm::IRBuilder builder(context);

    this->resolve_forward_references(module.get(), &builder);
    this->validate_ast_nodes();

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




