#include <llvm/ADT/STLExtras.h>
#include "../include/codegen.hpp"

namespace llscm {
    ScmCodeGen::ScmCodeGen(LLVMContext &ctxt, ScmProg * tree):
            context(ctxt), builder(ctxt), ast(tree) {
        module = make_unique<Module>("scm_module", context);
        initTypes();
        testAstVisit();
        addTestFunc();
    }

    void ScmCodeGen::initTypes() {
        vector<Type*> fields;
        Type * ti32 = builder.getInt32Ty();
        ArrayType * ti8arr = ArrayType::get(builder.getInt8Ty(), 1);
        PointerType * scm_type_ptr;
        PointerType * scm_fn_ptr;

        // %scm_type = type { i32 }
        t.scm_type = module->getTypeByName("scm_type");
        if (!t.scm_type) {
            t.scm_type = StructType::create(context, "scm_type");
            fields = { ti32 };
            t.scm_type->setBody(fields, false);
        }

        // %scm_int = type { i32, i64 }
        t.scm_int = module->getTypeByName("scm_int");
        if (!t.scm_int) {
            t.scm_int = StructType::create(context, "scm_int");
            fields = { ti32, builder.getInt64Ty() };
            t.scm_int->setBody(fields, false);
        }

        // %scm_float = type { i32, double }
        t.scm_float = module->getTypeByName("scm_float");
        if (!t.scm_float) {
            t.scm_float = StructType::create(context, "scm_float");
            fields = { ti32, builder.getDoubleTy() };
            t.scm_float->setBody(fields, false);
        }

        // %scm_str = type { i32, i32, [1 x i8] }
        t.scm_str = module->getTypeByName("scm_str");
        if (!t.scm_str) {
            t.scm_str = StructType::create(context, "scm_str");
            fields = { ti32, ti32, ti8arr };
            t.scm_str->setBody(fields, false);
        }

        // %scm_sym = type { i32, i32, [1 x i8] }
        t.scm_sym = module->getTypeByName("scm_sym");
        if (!t.scm_sym) {
            t.scm_sym = StructType::create(context, "scm_sym");
            fields = { ti32, ti32, ti8arr };
            t.scm_sym->setBody(fields, false);
        }

        // %scm_cons = type { i32, %scm_type*, %scm_type* }
        scm_type_ptr = PointerType::get(t.scm_type, 0);
        t.scm_cons = module->getTypeByName("scm_cons");
        if (!t.scm_cons) {
            t.scm_cons = StructType::create(context, "scm_cons");
            fields = { ti32, scm_type_ptr, scm_type_ptr};
            t.scm_cons->setBody(fields, false);
        }

        // %scm_func = type { i32, i32, %scm_type* (i32, ...)* }
        t.scm_fn_sig = FunctionType::get(scm_type_ptr, { ti32 }, true);
        scm_fn_ptr = PointerType::get(t.scm_fn_sig, 0);
        t.scm_func = module->getTypeByName("scm_func");
        if (!t.scm_func) {
            t.scm_func = StructType::create(context, "scm_func");
            fields = { ti32, ti32, scm_fn_ptr };
            t.scm_func->setBody(fields, false);
        }
    }

    void ScmCodeGen::addTestFunc() {
        Function * func = Function::Create(
                FunctionType::get(builder.getVoidTy(), {}, false),
                GlobalValue::ExternalLinkage,
                "func",
                module.get()
        );

        BasicBlock * entry = BasicBlock::Create(context, "entry", func);
        builder.SetInsertPoint(entry);
        builder.CreateAlloca(t.scm_type);
        builder.CreateAlloca(t.scm_int);
        builder.CreateAlloca(t.scm_float);
        builder.CreateAlloca(t.scm_str);
        builder.CreateAlloca(t.scm_sym);
        builder.CreateAlloca(t.scm_cons);
        builder.CreateAlloca(t.scm_func);
        builder.CreateRetVoid();
    }

    void ScmCodeGen::testAstVisit() {
        Value * code = codegen(ast);
        assert(code == nullptr);
    }

    any_ptr ScmCodeGen::visit(ScmProg *node) {
        // Test
        for (auto & e: *node) {
            e->printSrc(cerr);
            cerr << endl;
        }
        return any_ptr();
    }
}

