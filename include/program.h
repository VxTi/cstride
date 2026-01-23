#pragma once
#include "ast/scope.h"
#include "ast/nodes/ast_node.h"

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
        std::shared_ptr<ast::Scope> _global_scope;
        std::vector<std::unique_ptr<ProgramObject>> _program_objects;

        void print_ast_nodes() const;

    public:
        void parse_files(const std::vector<std::string>& files);

        void execute(int argc, char* argv[]) const;


        std::vector<std::unique_ptr<ProgramObject>>& nodes() { return _program_objects; }

        const std::vector<std::string>& get_files() const { return _files; }

        [[nodiscard]]
        std::shared_ptr<ast::Scope> get_global_scope() const { return this->_global_scope; }

        Program() = default;
        ~Program() = default;

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program(Program&&) noexcept = default;
        Program& operator=(Program&&) noexcept = default;

    private:
        void resolve_forward_references(
            llvm::Module* module,
            llvm::LLVMContext& context,
            llvm::IRBuilder<>* builder
        ) const;

        void generate_llvm_ir(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) const;
    };
}
