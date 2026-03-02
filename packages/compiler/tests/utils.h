#pragma once

#include "ast/parser.h"
#include "ast/parsing_context.h"
#include "ast/tokens/tokenizer.h"
#include "files.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/traversal.h"
#include "../include/ast/visitor.h"

#include <gtest/gtest.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>
#include <memory>
#include <string>
#include <llvm/IR/Module.h>

namespace stride::tests
{
    inline std::pair<std::unique_ptr<ast::AstBlock>, std::shared_ptr<ast::ParsingContext>> parse_code_with_context(
        const std::string& code)
    {
        const auto source = std::make_shared<SourceFile>("test.sr", code);
        auto tokens = ast::tokenizer::tokenize(source);
        const auto context = std::make_shared<ast::ParsingContext>();

        auto parsed = parse_sequential(context, tokens);

        ast::AstNodeTraverser traverser;
        ast::TypeInferenceVisitor type_visitor;
        ast::FunctionDeclareVisitor function_declare_visitor;

        traverser.visit(&function_declare_visitor, parsed.get());
        traverser.visit(&type_visitor, parsed.get());
        parsed->validate();

        return std::make_pair(std::move(parsed), context);
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

        llvm::LLVMContext llvm_context;
        llvm::Module module("test_module", llvm_context);
        llvm::IRBuilder<> builder(llvm_context);

        block->resolve_forward_references(context.get(), &module, &builder);
        block->validate();
        block->codegen(&module, &builder);
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
