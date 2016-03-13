#ifndef LLSCHEME_CODEGEN_HPP
#define LLSCHEME_CODEGEN_HPP

#include <memory>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include "ast_visitor.hpp"
#include "ast.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;

    class AstCGVisitor: public AstVisitor {
    public:
        virtual any_ptr visit(const ScmProg * node) const;

        inline Value * codegen(VisitableObj * node) {
            any_ptr ret = node->accept(this);
            return APC<Value>(ret);
        }
    };

    class ScmCodeGen {
        unique_ptr<Module> module;
        LLVMContext & context;
        IRBuilder<> builder;
        VisitableObj * ast;
        AstCGVisitor vis;

        struct {
            StructType * scm_type;
            StructType * scm_int;
            StructType * scm_float;
            StructType * scm_str;
            StructType * scm_sym;
            StructType * scm_cons;
            StructType * scm_func;
            FunctionType * scm_fn_sig;
        } t;

        void initTypes();
        void addTestFunc();
        void testAstVisit();
    public:
        ScmCodeGen(LLVMContext & ctxt, ScmProg * tree);
        void dump() {
            module->dump();
        }
    };
}


#endif //LLSCHEME_CODEGEN_HPP
