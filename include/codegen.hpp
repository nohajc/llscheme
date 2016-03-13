#ifndef LLSCHEME_CODEGEN_HPP
#define LLSCHEME_CODEGEN_HPP

#include <memory>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>

namespace llscm {
    using namespace std;
    using namespace llvm;

    class ScmCodeGen {
        unique_ptr<Module> module;
        LLVMContext & context;
        IRBuilder<> builder;

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
    public:
        ScmCodeGen(LLVMContext & ctxt);
        void dump() {
            module->dump();
        }
    };
}


#endif //LLSCHEME_CODEGEN_HPP
