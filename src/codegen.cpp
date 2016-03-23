#include <cassert>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/raw_ostream.h>
#include "../include/codegen.hpp"
#include "../include/debug.hpp"

namespace llscm {
    //const char * RuntimeSymbol::malloc = "scm_alloc";
    const char * RuntimeSymbol::cons = "scm_cons";
    const char * RuntimeSymbol::car = "scm_car";
    const char * RuntimeSymbol::cdr = "scm_cdr";
    const char * RuntimeSymbol::isNull = "scm_is_null";
    const char * RuntimeSymbol::plus = "scm_plus";
    const char * RuntimeSymbol::minus = "scm_minus";
    const char * RuntimeSymbol::times = "scm_times";
    const char * RuntimeSymbol::div = "scm_div";
    const char * RuntimeSymbol::gt = "scm_gt";
    const char * RuntimeSymbol::display = "scm_display";
    const char * RuntimeSymbol::num_eq = "scm_num_eq";
    const char * RuntimeSymbol::cmd_args = "scm_cmd_args";
    const char * RuntimeSymbol::vec_len = "scm_vector_length";
    const char * RuntimeSymbol::vec_ref = "scm_vector_ref";

    ScmCodeGen::ScmCodeGen(LLVMContext &ctxt, ScmProg * tree):
            context(ctxt), builder(ctxt), ast(tree) {
        module = make_shared<Module>("scm_module", context);
        initTypes();
        //testAstVisit();
        //addTestFunc();
        entry_func = nullptr;
        // TODO: Take these methods out of the constructor
        addMainFuncProlog();
        codegen(ast);
        addMainFuncEpilog();
        verifyFunction(*entry_func, &errs());
    }

    void ScmCodeGen::initTypes() {
        vector<Type*> fields;
        t.ti32 = builder.getInt32Ty();
        ArrayType * ti8arr = ArrayType::get(builder.getInt8Ty(), 1);
        PointerType * scm_fn_ptr;

        // %scm_type = type { i32 }
        t.scm_type = module->getTypeByName("scm_type");
        if (!t.scm_type) {
            t.scm_type = StructType::create(context, "scm_type");
            fields = { t.ti32 };
            t.scm_type->setBody(fields, false);
        }

        // %scm_int = type { i32, i64 }
        t.scm_int = module->getTypeByName("scm_int");
        if (!t.scm_int) {
            t.scm_int = StructType::create(context, "scm_int");
            fields = { t.ti32, builder.getInt64Ty() };
            t.scm_int->setBody(fields, false);
        }

        // %scm_float = type { i32, double }
        t.scm_float = module->getTypeByName("scm_float");
        if (!t.scm_float) {
            t.scm_float = StructType::create(context, "scm_float");
            fields = { t.ti32, builder.getDoubleTy() };
            t.scm_float->setBody(fields, false);
        }

        // %scm_str = type { i32, i32, [1 x i8] }
        t.scm_str = module->getTypeByName("scm_str");
        if (!t.scm_str) {
            t.scm_str = StructType::create(context, "scm_str");
            fields = { t.ti32, t.ti32, ti8arr };
            t.scm_str->setBody(fields, false);
        }

        // %scm_sym = type { i32, i32, [1 x i8] }
        t.scm_sym = module->getTypeByName("scm_sym");
        if (!t.scm_sym) {
            t.scm_sym = StructType::create(context, "scm_sym");
            fields = { t.ti32, t.ti32, ti8arr };
            t.scm_sym->setBody(fields, false);
        }

        // %scm_cons = type { i32, %scm_type*, %scm_type* }
        t.scm_type_ptr = PointerType::get(t.scm_type, 0);
        t.scm_cons = module->getTypeByName("scm_cons");
        if (!t.scm_cons) {
            t.scm_cons = StructType::create(context, "scm_cons");
            fields = { t.ti32, t.scm_type_ptr, t.scm_type_ptr};
            t.scm_cons->setBody(fields, false);
        }

        // %scm_func = type { i32, i32, %scm_type* (i32, ...)* }
        // TODO: every function does not have to be varargs
        t.scm_fn_sig = FunctionType::get(t.scm_type_ptr, { t.ti32 }, true);
        scm_fn_ptr = PointerType::get(t.scm_fn_sig, 0);
        t.scm_func = module->getTypeByName("scm_func");
        if (!t.scm_func) {
            t.scm_func = StructType::create(context, "scm_func");
            fields = { t.ti32, t.ti32, scm_fn_ptr };
            t.scm_func->setBody(fields, false);
        }

        // %scm_vec = type { i32, i32, [1 x %scm_type*] }
        t.scm_vec = module->getTypeByName("scm_vec");
        if (!t.scm_vec) {
            t.scm_vec = StructType::create(context, "scm_vec");
            fields = { t.ti32, t.ti32, ArrayType::get(t.scm_type_ptr, 1) };
            t.scm_vec->setBody(fields, false);
        }
    }

    template<>
    Constant * ScmCodeGen::initScmConstant<ScmCodeGen::INT>(vector<Constant*> & fields, int64_t && val) {
        D(cerr << "constant int" << endl);
        fields.push_back(builder.getInt64((uint64_t)val));
        return ConstantStruct::get(t.scm_int, fields);
    }

    template<>
    Constant * ScmCodeGen::initScmConstant<ScmCodeGen::FLOAT>(vector<Constant*> & fields, double && val) {
        D(cerr << "constant float" << endl);
        fields.push_back(ConstantFP::get(context, APFloat(val)));
        return ConstantStruct::get(t.scm_float, fields);
    }

    template<>
    Constant * ScmCodeGen::initScmConstant<ScmCodeGen::CONS>(vector<Constant*> & fields, Constant *&& car, Constant *&& cdr) {
        D(cerr << "constant cons" << endl);
        Constant * c_car = ConstantExpr::getCast(Instruction::BitCast, car, t.scm_type_ptr);
        Constant * c_cdr = ConstantExpr::getCast(Instruction::BitCast, cdr, t.scm_type_ptr);
        fields.push_back(c_car);
        fields.push_back(c_cdr);
        return ConstantStruct::get(t.scm_cons, fields);
    }

    StructType *ScmCodeGen::getScmStrType(Type * strt) {
        vector<Type*> fields = { t.ti32, t.ti32, strt };
        return StructType::get(context, fields);
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

    /*void ScmCodeGen::testAstVisit() {
        Value * code = codegen(ast);
        assert(code == nullptr);
    }*/

    void ScmCodeGen::addMainFuncProlog() {
        vector<Type*> main_args_type = {
                builder.getInt32Ty(),
                PointerType::get(builder.getInt8PtrTy(0), 0)
        };
        FunctionType * main_func_type = FunctionType::get(
                builder.getInt32Ty(),
                main_args_type,
                false
        );
        Function * main_func = Function::Create(
                main_func_type,
                GlobalValue::ExternalLinkage,
                "main", module.get()
        );

        main_func->setDoesNotThrow();
        main_func->setHasUWTable();

        auto args = main_func->arg_begin();
        Value * int_argc = args++;
        Value * ptr_argv = args++;
        int_argc->setName("argc");
        ptr_argv->setName("argv");
        // TODO: wrap arguments in Scm compatible objects

        BasicBlock * bb = BasicBlock::Create(context, "entry", main_func);
        builder.SetInsertPoint(bb);

        g_exit_code = new GlobalVariable(
                *module, builder.getInt32Ty(), false,
                GlobalValue::ExternalLinkage,
                builder.getInt32(0), "exit_code"
        );

        g_argv = new GlobalVariable(
                *module, t.scm_type_ptr, false,
                GlobalValue::ExternalLinkage,
                ConstantPointerNull::get(t.scm_type_ptr), "scm_argv"
        );

        FunctionType * get_argv_func_type = FunctionType::get(
                t.scm_type_ptr,
                main_args_type,
                false
        );

        Function * get_argv_func = Function::Create(
                get_argv_func_type,
                GlobalValue::ExternalLinkage,
                "scm_get_arg_vector", module.get()
        );

        Value * arg_vec = builder.CreateCall(get_argv_func, { int_argc, ptr_argv });
        builder.CreateStore(arg_vec, g_argv);

        /*LoadInst * exit_c = builder.CreateLoad(g_exit_code);
        builder.CreateRet(exit_c);
        builder.SetInsertPoint(exit_c);*/

        entry_func = main_func;
    }

    void ScmCodeGen::addMainFuncEpilog() {
        LoadInst * exit_c = builder.CreateLoad(g_exit_code);
        builder.CreateRet(exit_c);
        //builder.SetInsertPoint(exit_c);
    }


    any_ptr ScmCodeGen::visit(ScmProg * node) {
        D(cerr << "VISITED ScmProg!" << endl);
        for (auto & e: *node) {
            //e->printSrc(cerr);
            //cerr << endl;
            codegen(e);
        }
        return any_ptr();
    }

    any_ptr ScmCodeGen::visit(ScmInt * node) {
        D(cerr << "VISITED ScmInt!" << endl);
        Constant * c = getScmConstant<INT>(node->val);
        return node->IR_val = new GlobalVariable(
                *module, t.scm_int, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmFloat * node) {
        D(cerr << "VISITED ScmFloat!" << endl);
        Constant * c = getScmConstant<FLOAT>(node->val);
        return node->IR_val = new GlobalVariable(
                *module, t.scm_float, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmTrue * node) {
        D(cerr << "VISITED ScmTrue!" << endl);
        Constant * c = getScmConstant<TRUE>();
        return node->IR_val = new GlobalVariable(
                *module, t.scm_type, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmFalse * node) {
        D(cerr << "VISITED ScmFalse!" << endl);
        Constant * c = getScmConstant<FALSE>();
        return node->IR_val = new GlobalVariable(
                *module, t.scm_type, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmNull * node) {
        D(cerr << "VISITED ScmNull!" << endl);
        Constant * c = getScmConstant<NIL>();
        return node->IR_val = new GlobalVariable(
                *module, t.scm_type, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmStr * node) {
        D(cerr << "VISITED ScmStr!" << endl);
        // Each string has a different type according to its length.
        // That type must match with the global variable type.
        Constant * c = getScmConstant<STR>(node->val);
        Type * str_type = c->getAggregateElement(2)->getType();
        return node->IR_val = new GlobalVariable(
                *module, getScmStrType(str_type), true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmSym * node) {
        D(cerr << "VISITED ScmSym!" << endl);
        // Each string has a different type according to its length.
        // That type must match with the global variable type.
        Constant * c = getScmConstant<SYM>(node->val);
        Type * str_type = c->getAggregateElement(2)->getType();
        return node->IR_val = new GlobalVariable(
                *module, getScmStrType(str_type), true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmRef * node) {
        D(cerr << "VISITED ScmRef!" << endl);
        // TODO: We have to translate different kinds of Refs.
        // Direct access to locals and globals, indirect to closure variables.
        P_ScmObj robj = node->refObj();
        if (robj->is_global_var) {
            return builder.CreateLoad(robj->IR_val);
        }
        if (!robj->IR_val && robj->t == T_FUNC) {
            // Reference to native function
            assert(DPC<ScmFunc>(robj)->body_list == nullptr);
            return node->IR_val = codegen(robj);
        }
        // In other cases we always have a reference to something
        // for which the code was already generated.
        // Therefore the missing IR_val is most likely a bug.
        assert(robj->IR_val);
        return node->IR_val = robj->IR_val;
    }

    // This must be called on quoted lists only!
    // Other lists (arglists, bindlists) have different meaning
    // and they won't be translated into cons cells.
    any_ptr ScmCodeGen::visit(ScmCons * node) {
        D(cerr << "VISITED ScmCons!" << endl);
        Constant * car = dyn_cast<Constant>(codegen(node->car));
        Constant * cdr = dyn_cast<Constant>(codegen(node->cdr));
        assert(car);
        assert(cdr);

        Constant * c = getScmConstant<CONS>(car, cdr);
        return node->IR_val = new GlobalVariable(
                *module, t.scm_cons, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmFunc * node) {
        D(cerr << "VISITED ScmFunc!" << endl);
        if (node->IR_val) return node->IR_val;

        vector<Type*> arg_types;
        FunctionType * func_type;
        Function * func;
        bool varargs = false;
        auto saved_ip = builder.saveIP();

        if (node->argc_expected == ArgsAnyCount) {
            varargs = true;
            arg_types.push_back(t.ti32);
        }
        else {
            arg_types.insert(arg_types.end(), (uint32_t)node->argc_expected, t.scm_type_ptr);
        }
        func_type = FunctionType::get(t.scm_type_ptr, arg_types, varargs);
        // TODO: How to handle redefinitions?
        func = Function::Create(
                func_type, // TODO:
                GlobalValue::ExternalLinkage,
                node->name, module.get()
        );

        // Save function declaration
        node->IR_val = func;

        if (!node->arg_list) { // Return declaration only (in case of extern functions).
            return func;
        }

        auto arg_it = func->args().begin();
        DPC<ScmCons>(node->arg_list)->each([&arg_it](P_ScmObj e) {
            ScmRef * fn_arg = dynamic_cast<ScmRef*>(e.get());
            assert(fn_arg);
            fn_arg->refObj()->IR_val = arg_it;
            arg_it++;
        });

        BasicBlock * bb = BasicBlock::Create(context, "entry", func);
        Value * ret_val, * c_ret_val;
        builder.SetInsertPoint(bb);

        DPC<ScmCons>(node->body_list)->each([this, &ret_val](P_ScmObj e) {
            ret_val = codegen(e);
        });
        c_ret_val = builder.CreateBitCast(ret_val, t.scm_type_ptr);
        builder.CreateRet(c_ret_val);
        verifyFunction(*func, &errs());

        builder.restoreIP(saved_ip);
        return func;
    }

    any_ptr ScmCodeGen::visit(ScmCall * node) {
        D(cerr << "VISITED ScmCall!" << endl);
        if (node->indirect) {
            // TODO: emit code for several runtime checks of the function pointer
            return AstVisitor::visit(node);
        }
        else {
            vector<Value*> args;
            ScmRef * fn_ref = dynamic_cast<ScmRef*>(node->fexpr.get());
            assert(fn_ref);
            ScmFunc * fn_obj = dynamic_cast<ScmFunc*>(fn_ref->refObj().get());
            assert(fn_obj);
            Function * func = dyn_cast<Function>(codegen(fn_obj));

            if (node->arg_list->t != T_NULL) {
                // At least one argument
                ScmCons * arg_list = dynamic_cast<ScmCons*>(node->arg_list.get());
                if (fn_obj->argc_expected == ArgsAnyCount) {
                    args.push_back(builder.getInt32((uint32_t)arg_list->length()));
                }

                arg_list->each([this, &args](P_ScmObj e) {
                    Value * a = codegen(e);
                    if (a->getType() != t.scm_type_ptr) {
                        a = builder.CreateBitCast(a, t.scm_type_ptr);
                    }
                    args.push_back(a);
                });
            }

            return node->IR_val = builder.CreateCall(func, args, fn_obj->name);
        }
    }

    any_ptr ScmCodeGen::visit(ScmDefineVarSyntax * node) {
        D(cerr << "VISITED ScmDefineVarSyntax!" << endl);
        if (node->val->t == T_FUNC || node->val->t == T_REF) {
            return codegen(node->val);
        }

        Value * expr = codegen(node->val);
        PointerType * etype = dyn_cast<PointerType>(expr->getType());
        assert(etype);

        // TODO: We should not always create global variables.
        // That's needed for top-level definitions only.
        // So we need a flag which tells us whether
        // the definition is top-level or not.
        Value * gvar = new GlobalVariable(
                *module, etype, false,
                GlobalValue::InternalLinkage,
                ConstantPointerNull::get(etype), ""
        );

        // Save expr to global var
        builder.CreateStore(expr, gvar);
        node->val->is_global_var = true;
        return node->val->IR_val = gvar;
    }

    any_ptr ScmCodeGen::visit(ScmIfSyntax * node) {
        D(cerr << "VISITED ScmIfSyntax!" << endl);
        Value * cond = codegen(node->cond_expr);
        vector<Value*> indices(2, builder.getInt32(0));
        Value * cond_arg_addr = builder.CreateGEP(cond, indices);
        Value * cond_tag = builder.CreateLoad(cond_arg_addr);
        cond = builder.CreateICmpNE(cond_tag, builder.getInt32(FALSE));

        Function * func = builder.GetInsertBlock()->getParent();

        BasicBlock * then_bb = BasicBlock::Create(context, "then", func);
        BasicBlock * else_bb = BasicBlock::Create(context, "else");
        BasicBlock * merge_bb = BasicBlock::Create(context, "merge");

        builder.CreateCondBr(cond, then_bb, else_bb);

        builder.SetInsertPoint(then_bb);
        Value * then_ret = codegen(node->then_expr);
        then_ret = builder.CreateBitCast(then_ret, t.scm_type_ptr);
        builder.CreateBr(merge_bb);
        then_bb = builder.GetInsertBlock();

        func->getBasicBlockList().push_back(else_bb);
        builder.SetInsertPoint(else_bb);
        Value * else_ret = codegen(node->else_expr);
        else_ret = builder.CreateBitCast(else_ret, t.scm_type_ptr);
        builder.CreateBr(merge_bb);
        else_bb = builder.GetInsertBlock();

        func->getBasicBlockList().push_back(merge_bb);
        builder.SetInsertPoint(merge_bb);
        PHINode * phi = builder.CreatePHI(t.scm_type_ptr, 2, "ifres");
        phi->addIncoming(then_ret, then_bb);
        phi->addIncoming(else_ret, else_bb);

        return node->IR_val = phi;
    }

    any_ptr ScmCodeGen::visit(ScmLetSyntax * node) {
        D(cerr << "VISITED ScmLetSyntax!" << endl);
        // Generate code for binding list expression evaluation
        if (node->bind_list->t != T_NULL) {
            DPC<ScmCons>(node->bind_list)->each([this](P_ScmObj e) {
                assert(e->t == T_CONS);
                shared_ptr<ScmCons> kv = DPC<ScmCons>(e);
                assert(kv->cdr->t == T_CONS);
                P_ScmObj expr = DPC<ScmCons>(kv->cdr)->car;
                codegen(expr);
            });
        }

        Value * ret_val;

        DPC<ScmCons>(node->body_list)->each([this, &ret_val](P_ScmObj e) {
            ret_val = codegen(e);
        });

        return node->IR_val = ret_val;
    }

    any_ptr ScmCodeGen::visit(ScmQuoteSyntax * node) {
        D(cerr << "VISITED ScmQuoteSyntax!" << endl);
        return node->IR_val = codegen(node->data);
    }
}

