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

    class ScmCodeGen: public AstVisitor {
        unique_ptr<Module> module;
        LLVMContext & context;
        IRBuilder<> builder;
        VisitableObj * ast;

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

        virtual any_ptr visit(ScmProg * node);

        inline Value * codegen(VisitableObj * node) {
            any_ptr ret = node->accept(this);
            return APC<Value>(ret);
        }
    public:
        ScmCodeGen(LLVMContext & ctxt, ScmProg * tree);
        void dump() {
            module->dump();
        }
    };
}


#endif //LLSCHEME_CODEGEN_HPP
