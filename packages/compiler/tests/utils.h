#pragma once

#include "files.h"
#include "ast/ast.h"
#include "ast/symbol_table.h"
#include "ast/visitor.h"
#include "ast/nodes/blocks.h"
#include "../include/ast/traversal.h"
#include "ast/tokens/tokenizer.h"
#include "runtime/symbols.h"

#include <memory>
#include <string>
#include <gtest/gtest.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

namespace stride::tests
{
    inline std::pair<std::unique_ptr<ast::AstBlock>, std::shared_ptr<ast::SymbolTable>> parse_code_with_context(
        const std::string& code)
    {
        const auto source = std::make_shared<SourceFile>("test.sr", code);
        auto tokens = ast::tokenizer::tokenize(source);
        const auto context = std::make_shared<ast::SymbolTable>();

        auto node = parse_sequential(tokens);
        node->set_symbol_table(context);

        ast::AstNodeTraverser traverser(context);
        ast::TypeInferenceVisitor type_visitor;
        ast::SymbolResolver symbol_resolver;
        ast::GenericFunctionInstantiator generic_function_instantiator;
        ast::ImportVisitor import_visitor;

        // Populate symbol table with stride runtime symbols
        runtime::register_runtime_symbols(context.get());

        ast::AstBranch branch(source, std::move(node));

        // First step - Cross-file symbol registration (imports and function signatures)
        import_visitor.set_current_file_name("test.sr");
        traverser.traverse(&import_visitor, &branch);
        traverser.traverse(&symbol_resolver, &branch);

        // Normally we'd call cross_register_symbols here, but for a single file test,
        // it's usually not needed unless it's testing multi-file.

        // Generic template resolution
        traverser.traverse(&generic_function_instantiator, &branch);

        // Third step - Type resolution
        traverser.traverse(&type_visitor, &branch);

        return std::make_pair(branch.move_node(), context);
    }

    inline std::unique_ptr<ast::AstBlock> parse_code(const std::string& code)
    {
        return parse_code_with_context(code).first;
    }

    inline void assert_parses(const std::string& code)
    {
        EXPECT_NO_THROW({
            const auto block = parse_code(code);
            EXPECT_NE(block, nullptr) << "Parsing returned null for code: " << code;
            });
    }

    inline void assert_compiles(const std::string& code)
    {
        auto [block, context] = parse_code_with_context(code);
        EXPECT_NE(block, nullptr) << "Parsing returned null for code: " << code;

        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        auto jtmb = llvm::cantFail(llvm::orc::JITTargetMachineBuilder::detectHost());
        const auto target_machine = llvm::cantFail(jtmb.createTargetMachine());

        llvm::LLVMContext llvm_context;
        auto module = std::make_unique<llvm::Module>("test_module", llvm_context);
        module->setDataLayout(target_machine->createDataLayout());
        module->setTargetTriple(target_machine->getTargetTriple());
        llvm::IRBuilder<> builder(llvm_context);

        ast::AstNodeTraverser traverser(context);
        ast::ValidationVisitor validation_visitor;
        const auto source = std::make_shared<SourceFile>("test.sr", code);
        ast::AstBranch branch(source, std::move(block));

        traverser.traverse(&validation_visitor, &branch);

        branch.get_node()->resolve_forward_references(context.get(), module.get(), &builder);
        branch.get_node()->codegen(context.get(), module.get(), &builder);

        if (llvm::verifyModule(*module, &llvm::errs()))
        {
            module->print(llvm::errs(), nullptr);
            throw std::runtime_error("LLVM IR verification failed");
        }
    }

    inline void assert_throws(const std::string& code)
    {
        EXPECT_ANY_THROW({ assert_compiles(code); });
    }

    inline void assert_throws_message(
        const std::string& code,
        const std::string& contains_message
    )
    {
        try
        {
            assert_compiles(code);
            FAIL() << "Expected exception with message: \"" << contains_message
                << "\", but no exception was thrown.\nSource:\n"
                << code;
        }
        catch (const std::exception& e)
        {
            if (const std::string error_message = e.what();
                error_message.find(contains_message) == std::string::npos)
            {
                FAIL() << "Expected exception with message: \"" <<
                    contains_message
                    << "\", but got: \"" << error_message << "\"\nSource:\n"
                    << code;
            }
        }
        catch (...)
        {
            FAIL() << "Expected exception containing \"" << contains_message
                << "\", but an unknown exception type was thrown.\nSource:\n"
                << code;
        }
    }
} // namespace stride::tests
