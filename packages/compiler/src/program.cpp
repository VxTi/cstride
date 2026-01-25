#include "program.h"

#include <iostream>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/Support/TargetSelect.h>

#include "ast/parser.h"
#include "ast/nodes/functions.h"
#include "stl/stl_functions.h"

using namespace stride;

void Program::parse_files(const std::vector<std::string>& files)
{
    this->_global_scope = std::make_shared<ast::Scope>(ast::ScopeType::GLOBAL);
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

void Program::optimize_ast_nodes() const
{
    std::vector<std::unique_ptr<ast::IAstNode>> new_children;
    for (int i = 0; i < this->_root_node->children().capacity(); i++)
    {
        ast::IAstNode* child = this->_root_node->children()[i].get();

        if (auto* reducible = dynamic_cast<ast::IReducible*>(child))
        {
            if (auto reduced = reducible->reduce())
            {
                std::cout << "Reduced node: " << reduced->to_string() << std::endl;
                new_children.push_back(
                    std::vector<std::unique_ptr<ast::IAstNode>>::value_type(reduced)
                );
            }
        }
        else
        {
            // Keep previous node
            new_children.push_back(std::unique_ptr<ast::IAstNode>(child));
        }
    }

    const auto new_root = std::make_unique<ast::AstBlock>(
        this->_root_node->source,
        this->_root_node->source_offset,
        this->get_global_scope(),
        std::move(new_children)
    );
    // TODO:
    //    this->_root_node = new_root.get();
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

void Program::compile_jit() const
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    // 1. Initialize LLVM targets for the host machine
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // 2. Setup Context and JIT instance
    // ORC JIT requires a ThreadSafeContext to manage the lifetime of the IR
    const auto thread_safe_context = std::make_unique<llvm::orc::ThreadSafeContext>(
        std::make_unique<llvm::LLVMContext>());
    auto* context = thread_safe_context->withContextDo([](llvm::LLVMContext* Ctx) { return Ctx; });

    auto jtmb_expected = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!jtmb_expected)
    {
        throw std::runtime_error("Failed to detect host target");
    }
    auto jtmb = std::move(*jtmb_expected);

    auto jit_expected = llvm::orc::LLJITBuilder()
                       .setJITTargetMachineBuilder(std::move(jtmb))
                       .create();
    if (!jit_expected)
    {
        throw std::runtime_error("Failed to create LLJIT instance");
    }
    const auto jit = std::move(*jit_expected);

    // Add search generator for the current process
    jit->getMainJITDylib().addGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            jit->getDataLayout().getGlobalPrefix()))
    );

    auto module = std::make_unique<llvm::Module>("stride_jit_module", *context);
    module->setDataLayout(jit->getDataLayout());
    module->setTargetTriple(llvm::Triple(jit->getTargetTriple().str()));

    llvm::IRBuilder<> builder(*context);

    stl::llvm_jit_define_functions(jit.get());
    stl::llvm_insert_function_definitions(module.get(), *context);

    // 3. Setup Module with correct DataLayout and Triple

    // 4. Generate IR (Reusing your existing logic)
    this->validate_ast_nodes();
    this->resolve_forward_references(module.get(), *context, &builder);
    this->generate_llvm_ir(module.get(), *context, &builder);

    if (llvm::verifyModule(*module, &llvm::errs()))
    {
        throw std::runtime_error("LLVM IR verification failed for JIT");
    }

    // 5. Add the IR Module to the JIT
    // We wrap our module in a ThreadSafeModule before handing it over
    auto tsm = llvm::orc::ThreadSafeModule(std::move(module), std::move(*thread_safe_context));
    if (auto err = jit->addIRModule(std::move(tsm)))
    {
        throw std::runtime_error("Failed to add IR module to JIT");
    }

    // Locate the main function of the process
    auto main_symbol = jit->lookup(MAIN_FN_NAME);
    if (!main_symbol)
    {
        throw std::runtime_error("JIT: Could not find 'main' function in generated IR");
    }

    // Execute the main function of the process
    auto* main_fn = main_symbol->toPtr<int (*)()>();

    std::cout << "--- Starting JIT Execution ---" << std::endl;
    const int result = main_fn();
    std::cout << "--- JIT Finished with exit code: " << result << " ---" << std::endl;
}
