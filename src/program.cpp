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
#include "ast/nodes/functions.h"

using namespace stride;

void Program::parse_files(const std::vector<std::string>& files)
{
    this->_global_scope = std::make_shared<ast::Scope>(ast::ScopeType::GLOBAL);
    this->_files = files;

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
                    std::vector<std::unique_ptr<ast::IAstNode>>::value_type(std::move(reduced))
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

void Program::execute(int argc, char* argv[]) const
{
    setvbuf(stdout, nullptr, _IONBF, 0);

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
        llvm::sys::getHostCPUName(),
        "",
        llvm::TargetOptions(),
        std::optional<llvm::Reloc::Model>()
    );

    module->setDataLayout(machine->createDataLayout());
    module->setTargetTriple(triple);

    this->validate_ast_nodes();
    // this->optimize_ast_nodes();

    // Ensures symbols are defined for codegen
    this->resolve_forward_references(module.get(), context, &builder);
    this->generate_llvm_ir(module.get(), context, &builder);

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
                                   .setMCPU(llvm::sys::getHostCPUName())
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
