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

namespace llscm {
    using namespace std;
    using namespace llvm;

    struct RuntimeSymbol {
        //static const char *malloc;
        static const char *cons;
        static const char *car;
        static const char *cdr;
        static const char *isNull;
        static const char *plus;
        static const char *minus;
        static const char *times;
        static const char *div;
        static const char *gt;
        static const char *display;
        static const char *num_eq;
    };

    class ScmCodeGen: public AstVisitor {
        shared_ptr<Module> module;
        LLVMContext & context;
        IRBuilder<> builder;
        VisitableObj * ast;
        Function * entry_func;

        struct {
            StructType * scm_type;
            StructType * scm_int;
            StructType * scm_float;
            StructType * scm_str;
            StructType * scm_sym;
            StructType * scm_cons;
            StructType * scm_func;
            FunctionType * scm_fn_sig;
            PointerType * scm_type_ptr;
            Type * ti32;
        } t;

        enum Tag {
            FALSE, TRUE, NIL, INT, FLOAT, STR, SYM, CONS, FUNC
        };

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
        void addMainFunc(); // Called when we want to compile a standalone app
        void addTestFunc();
        //void testAstVisit();

        /*
            class ScmProg;
            class ScmObj;
            class ScmInt;
            class ScmFloat;
            class ScmTrue;
            class ScmFalse;
            class ScmNull;
            class ScmStr;
            class ScmSym;
            class ScmRef;
            class ScmCons;
            class ScmFunc;
            class ScmCall;
            class ScmDefineVarSyntax;
            class ScmDefineFuncSyntax; // NOT PRESENT IN AST AFTER CT_Eval
            class ScmLambdaSyntax; // NOT PRESENT IN AST AFTER CT_Eval
            class ScmQuoteSyntax;
            class ScmIfSyntax;
            class ScmLetSyntax;
        */

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
        Module * getModule() {
            return module.get();
        }

        LLVMContext & getContext() {
            return context;
        }
    };
}


#endif //LLSCHEME_CODEGEN_HPP
