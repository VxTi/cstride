#pragma once
#include "cli.h"
#include "ast/ast.h"
#include "ast/parsing_context.h"
#include "ast/nodes/ast_node.h"
#include "ast/nodes/blocks.h"

#include <llvm/Target/TargetMachine.h>

namespace stride
{
    class ProgramObject
    {
        std::unique_ptr<ast::IAstNode> _root;

    public:
        explicit ProgramObject(std::unique_ptr<ast::IAstNode> root) :
            _root(std::move(root)) {}

        [[nodiscard]]
        ast::IAstNode* get_root_ast_node() const
        {
            return _root.get();
        }

        ~ProgramObject() = default;

        ProgramObject(const ProgramObject&) = delete;
        ProgramObject& operator=(const ProgramObject&) = delete;
        ProgramObject(ProgramObject&&) noexcept = default;
        ProgramObject& operator=(ProgramObject&&) noexcept = default;
    };

    class Program
    {
        std::unique_ptr<ast::Ast> _ast;

        explicit Program(std::unique_ptr<ast::Ast> ast) :
            _ast(std::move(ast)) {}

    public:
        static Program from_sources(const std::vector<std::string>& files);

        ~Program() = default;

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

        [[nodiscard]]
        int compile_jit(const cli::CompilationOptions& options) const;

        [[nodiscard]]
        int compile(const cli::CompilationOptions& options) const;

        [[nodiscard]]
        ast::Ast* get_ast() const
        {
            return this->_ast.get();
        }

    private:
        std::unique_ptr<llvm::Module> prepare_module(
            llvm::LLVMContext& context,
            const cli::CompilationOptions& options,
            llvm::TargetMachine* target_machine
        ) const;
    };
} // namespace stride
