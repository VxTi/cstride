#pragma once
#include "ast/context.h"
#include "ast/nodes/ast_node.h"
#include "ast/nodes/blocks.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/Target/TargetMachine.h>

#include "cli.h"

namespace stride
{
    class ProgramObject
    {
        std::unique_ptr<ast::IAstNode> _root;

    public:
        explicit ProgramObject(std::unique_ptr<ast::IAstNode> root) :
            _root(std::move(root)) {}

        [[nodiscard]]
        ast::IAstNode* get_root_ast_node() const { return _root.get(); }

        ~ProgramObject() = default;

        ProgramObject(const ProgramObject&) = delete;
        ProgramObject& operator=(const ProgramObject&) = delete;
        ProgramObject(ProgramObject&&) noexcept = default;
        ProgramObject& operator=(ProgramObject&&) noexcept = default;
    };

    class Program
    {
        std::vector<std::string> _files;
        std::shared_ptr<ast::ParsingContext> _global_scope;
        std::unique_ptr<ast::AstBlock> _root_node;

    public:
        explicit Program() = default;

        void parse_files(std::vector<std::string> files);

        ~Program() = default;

        [[nodiscard]]
        std::shared_ptr<ast::ParsingContext> get_global_scope() const { return this->_global_scope; }

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

        [[nodiscard]]
        int compile_jit(const cli::CompilationOptions& options) const;

    private:
        void print_ast_nodes() const;

        void resolve_forward_references(
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) const;

        void validate_ast_nodes() const;

        void optimize_ast_nodes();

        void generate_llvm_ir(
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) const;
    };
}
