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
#include <llvm/Support/TargetSelect.h>

namespace stride::tests
{
    inline std::pair<std::unique_ptr<ast::AstBlock>, std::shared_ptr<ast::SymbolTable>> parse_code_with_context(
        const std::string& code)
    {
        const auto source = std::make_shared<SourceFile>("test.sr", code);
        auto tokens = ast::tokenizer::tokenize(source);
        const auto context = std::make_shared<ast::SymbolTable>();

        auto node = parse_sequential(context, tokens);

        ast::AstNodeTraverser traverser;
        ast::TypeInferenceVisitor expression_visitor;
        ast::SymbolResolver function_visitor;
        ast::FunctionCallVisitor function_call_visitor;
        ast::ImportVisitor import_visitor;

        runtime::register_runtime_symbols(node->get_symbol_table());

        import_visitor.set_current_file_name("test.sr");
        traverser.visit_block(&import_visitor, node.get());

        traverser.visit_block(&function_visitor, node.get());
        traverser.visit_block(&function_call_visitor, node.get());

        traverser.visit_block(&expression_visitor, node.get());

        node->validate(symbol_table);

        return std::make_pair(std::move(node), context);
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

        auto jtmb = llvm::cantFail(llvm::orc::JITTargetMachineBuilder::detectHost());
        const auto target_machine = llvm::cantFail(jtmb.createTargetMachine());

        llvm::LLVMContext llvm_context;
        llvm::Module module("test_module", llvm_context);
        module.setDataLayout(target_machine->createDataLayout());
        module.setTargetTriple(target_machine->getTargetTriple());
        llvm::IRBuilder<> builder(llvm_context);

        block->resolve_forward_references(symbol_table, &module, &builder);
        block->validate(symbol_table);
        block->codegen(symbol_table, &module, &builder);
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
