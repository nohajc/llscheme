#ifndef LLSCHEME_CODEGEN_HPP
#define LLSCHEME_CODEGEN_HPP

#include <memory>
#include <vector>
#include <utility>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include "ast_visitor.hpp"
#include "ast.hpp"
#include "debug.hpp"
#include "runtime/types.hpp"
#include "../include/libmetainfo.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;

    struct RuntimeSymbol {
        static const char *cons;
        static const char *car;
        static const char *cdr;
        static const char *is_null;
        static const char *plus;
        static const char *minus;
        static const char *times;
        static const char *div;
        static const char *gt;
        static const char *display;
        static const char *num_eq;
        static const char *cmd_args;
        static const char *vec_len;
        static const char *vec_ref;
        static const char *get_arg_vec;
        static const char *argv;
        static const char *exit_code;
        static const char *alloc_heap_storage;
        static const char *alloc_func;
        static const char *error_not_a_func;
        static const char *error_wrong_arg_num;
        static const char *apply;
        static const char *length;
        static const char *eval;
        static const char *make_base_nspace;
        static const char *current_nspace;
        static const char *read;
        static const char *is_eof;
        static const char *bool_and;
        static const char *bool_or;
        static const char *list;
        static const char *string_to_symbol;
        static const char *string_equals;
        static const char *string_append;
        static const char *string_replace;
        static const char *string_split;
    };

    class ScmCodeGen: public AstVisitor {
        LLVMContext & context;
        shared_ptr<Module> module;
        IRBuilder<> builder;
        VisitableObj * ast;
        Metadata output_meta;

        GlobalVariable * g_exit_code;
        GlobalVariable * g_argv;
        // Library constructors (called on module load)
        GlobalVariable * g_ctors;

        Function * entry_func;
        string entry_func_name;
        void (ScmCodeGen::*addEntryFuncProlog)();
        void (ScmCodeGen::*addEntryFuncEpilog)(Value *);

        struct {
            StructType * scm_type;
            StructType * scm_int;
            StructType * scm_float;
            StructType * scm_str;
            StructType * scm_sym;
            StructType * scm_cons;
            StructType * scm_func;
            StructType * scm_vec;
            FunctionType * scm_fn_sig;
            FunctionType * scm_wrfn_sig;
            PointerType * scm_fn_ptr;
            PointerType * scm_wrfn_ptr;
            PointerType * scm_type_ptr;
            Type * ti32;
        } t;

        struct {
            Function * alloc_func;
            Function * alloc_heap_storage;
            Function * error_not_a_func;
            Function * error_wrong_arg_num;
        } fn;

        template<Tag tag, typename ...Args>
        Constant * getScmConstant(Args ...args) {
            vector<Constant*> fields;
            fields.push_back(builder.getInt32(tag));
            return initScmConstant<tag>(fields, std::forward<Args>(args)...);
        };

        template<Tag tag, typename ...Args>
        inline Constant * initScmConstant(vector<Constant*> & fields, Args && ...args) {
            D(cerr << "WE DON'T WANT TO END UP HERE" << endl);
            assert(0);
            return nullptr;
        };

        template<Tag tag>
        Constant * initScmConstant(vector<Constant*> & fields) {
            D(cerr << "empty args" << endl);
            return ConstantStruct::get(t.scm_type, fields);
        }

        template<Tag tag>
        Constant * initScmConstant(vector<Constant*> & fields, string && val) {
            Constant * str = ConstantDataArray::getString(context, val, true);
            D(cerr << "constant string/symbol" << endl);
            fields.push_back(builder.getInt32((uint32_t)val.length()));
            fields.push_back(str);
            Constant * c = ConstantStruct::get(getScmStrType(str->getType()), fields);
            return c;
        }

        StructType * getScmStrType(Type * t);

        void initTypes();
        void initExternFuncs();

        void addMainFuncProlog(); // Called when we want to compile a standalone app
        void addMainFuncEpilog(Value * last_val);

        void addLibInitFuncProlog();
        void addLibInitFuncEpilog(Value * last_val);

        void addExprFuncProlog();
        void addExprFuncEpilog(Value * last_val);

        Value * genGlobalConstant(Constant * c);

        Value * genAllocHeapStorage(int32_t size);
        void genHeapStore(Value * hs, Value * obj, int32_t idx);
        Value * genHeapLoad(Value * hs, int32_t idx);

        Value * genAllocFunc(int32_t argc, Function * fnptr,
                             Function * wrfnptr, Value * ctxptr);
        Value * genConstFunc(int32_t argc, Function * fnptr, Function * wrfnptr);
        vector<Value*> genArgValues(const ScmCall * node);
        //void testAstVisit();
        template<typename F1, typename F2, typename F3>
        Value * genIfElse(F1 cond_expr, F2 then_expr, F3 else_expr) {
            Value * cond_val = cond_expr();
            Function * func = builder.GetInsertBlock()->getParent();

            BasicBlock * then_bb = BasicBlock::Create(context, "then", func);
            BasicBlock * else_bb = BasicBlock::Create(context, "else");
            BasicBlock * merge_bb = BasicBlock::Create(context, "merge");

            builder.CreateCondBr(cond_val, then_bb, else_bb);

            builder.SetInsertPoint(then_bb);
            Value * then_val = then_expr();
            builder.CreateBr(merge_bb);
            then_bb = builder.GetInsertBlock();

            func->getBasicBlockList().push_back(else_bb);
            builder.SetInsertPoint(else_bb);
            Value * else_val = else_expr();
            builder.CreateBr(merge_bb);
            else_bb = builder.GetInsertBlock();

            func->getBasicBlockList().push_back(merge_bb);

            builder.SetInsertPoint(merge_bb);
            PHINode * phi = builder.CreatePHI(then_val->getType(), 2, "ifres");
            phi->addIncoming(then_val, then_bb);
            phi->addIncoming(else_val, else_bb);

            return phi;
        }

        Value * genAndExpr(ScmCons * cell);
        Value * genOrExpr(ScmCons * cell);

        void declFuncWrapper(ScmFunc * node, GlobalValue::LinkageTypes linkage);
        void defFuncWrapper(ScmFunc * node, Function * func);

        virtual any_ptr visit(ScmProg * node);
        virtual any_ptr visit(ScmInt * node);
        virtual any_ptr visit(ScmFloat * node);
        virtual any_ptr visit(ScmTrue * node);
        virtual any_ptr visit(ScmFalse * node);
        virtual any_ptr visit(ScmNull * node);
        virtual any_ptr visit(ScmStr * node);
        virtual any_ptr visit(ScmSym * node);
        virtual any_ptr visit(ScmRef * node);
        virtual any_ptr visit(ScmCons * node);
        virtual any_ptr visit(ScmFunc * node);
        virtual any_ptr visit(ScmCall * node);
        virtual any_ptr visit(ScmDefineVarSyntax * node);
        virtual any_ptr visit(ScmQuoteSyntax * node);
        virtual any_ptr visit(ScmIfSyntax * node);
        virtual any_ptr visit(ScmLetSyntax * node);
        virtual any_ptr visit(ScmAndSyntax * node);
        virtual any_ptr visit(ScmOrSyntax * node);

        inline Value * codegen(VisitableObj * node) {
            any_ptr ret = node->accept(this);
            return APC<Value>(ret);
        }

        inline Value * codegen(P_ScmObj node) {
            return codegen(node.get());
        }
    public:
        ScmCodeGen(LLVMContext & ctxt, ScmProg * tree);
        void dump() {
            module->dump();
        }

        void run();

        void makeExecutable() {
            addEntryFuncProlog = &ScmCodeGen::addMainFuncProlog;
            addEntryFuncEpilog = &ScmCodeGen::addMainFuncEpilog;
        }

        void makeExpression(const string & name) {
            entry_func_name = name;
            addEntryFuncProlog = &ScmCodeGen::addExprFuncProlog;
            addEntryFuncEpilog = &ScmCodeGen::addExprFuncEpilog;
        }

        shared_ptr<Module> getModule() {
            return module;
        }

        LLVMContext & getContext() {
            return context;
        }
    };
}


#endif //LLSCHEME_CODEGEN_HPP
