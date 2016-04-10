#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/raw_ostream.h>
#include "../include/codegen.hpp"
#include "../include/debug.hpp"

namespace llscm {
    const char * RuntimeSymbol::cons = "scm_cons";
    const char * RuntimeSymbol::car = "scm_car";
    const char * RuntimeSymbol::cdr = "scm_cdr";
    const char * RuntimeSymbol::is_null = "scm_is_null";
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
    const char * RuntimeSymbol::get_arg_vec = "scm_get_arg_vector";
    const char * RuntimeSymbol::argv = "scm_argv";
    const char * RuntimeSymbol::exit_code = "exit_code";
    const char * RuntimeSymbol::alloc_heap_storage = "alloc_heap_storage";
    const char * RuntimeSymbol::alloc_func = "alloc_func";
    const char * RuntimeSymbol::error_not_a_func = "error_not_a_function";
    const char * RuntimeSymbol::error_wrong_arg_num = "error_wrong_arg_num";
    const char * RuntimeSymbol::apply = "scm_apply";
    const char * RuntimeSymbol::length = "scm_length";
    const char * RuntimeSymbol::eval = "scm_eval";
    const char * RuntimeSymbol::make_base_nspace = "scm_make_base_nspace";
    const char * RuntimeSymbol::read = "scm_read";
    const char * RuntimeSymbol::is_eof = "scm_is_eof";

    ScmCodeGen::ScmCodeGen(LLVMContext &ctxt, ScmProg * tree):
            context(ctxt), builder(ctxt), ast(tree) {
        module = make_shared<Module>("scm_module", context);
        initTypes();
        initExternFuncs();
        entry_func = nullptr;

        // Not adding main function by default
        addEntryFuncProlog = &ScmCodeGen::addLibInitFuncProlog;
        addEntryFuncEpilog = &ScmCodeGen::addLibInitFuncEpilog;
    }

    void ScmCodeGen::initTypes() {
        vector<Type*> fields;
        t.ti32 = builder.getInt32Ty();
        ArrayType * ti8arr = ArrayType::get(builder.getInt8Ty(), 1);

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

        // %scm_func = type { i32, i32, %scm_type* (%scm_type*, ...)*, %scm_type* (%scm_type**)*, %scm_type** }
        // Every function won't be varargs. This is just the default.
        t.scm_fn_sig = FunctionType::get(t.scm_type_ptr, { t.scm_type_ptr }, true);
        t.scm_fn_ptr = PointerType::get(t.scm_fn_sig, 0);
        // For each function, we provide a wrapper that is used when the number of arguments
        // passed to it is unknown at compile time (in case of scheme's apply function for example).
        // The wrapper always takes a pointer to array of the actual arguments.
        t.scm_wrfn_sig = FunctionType::get(
                t.scm_type_ptr, { PointerType::get(t.scm_type_ptr, 0) }, false
        );
        t.scm_wrfn_ptr = PointerType::get(t.scm_wrfn_sig, 0);

        // The scm_func structure therefore contains two function pointers.
        t.scm_func = module->getTypeByName("scm_func");
        if (!t.scm_func) {
            t.scm_func = StructType::create(context, "scm_func");
            fields = { t.ti32, t.ti32, t.scm_fn_ptr, t.scm_wrfn_ptr, PointerType::get(t.scm_type_ptr, 0) };
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
    Constant * ScmCodeGen::initScmConstant<S_INT>(vector<Constant*> & fields, int64_t && val) {
        D(cerr << "constant int" << endl);
        fields.push_back(builder.getInt64((uint64_t)val));
        return ConstantStruct::get(t.scm_int, fields);
    }

    template<>
    Constant * ScmCodeGen::initScmConstant<S_FLOAT>(vector<Constant*> & fields, double && val) {
        D(cerr << "constant float" << endl);
        fields.push_back(ConstantFP::get(context, APFloat(val)));
        return ConstantStruct::get(t.scm_float, fields);
    }

    template<>
    Constant * ScmCodeGen::initScmConstant<S_CONS>(vector<Constant*> & fields, Constant *&& car, Constant *&& cdr) {
        D(cerr << "constant cons" << endl);
        Constant * c_car = ConstantExpr::getCast(Instruction::BitCast, car, t.scm_type_ptr);
        Constant * c_cdr = ConstantExpr::getCast(Instruction::BitCast, cdr, t.scm_type_ptr);
        fields.push_back(c_car);
        fields.push_back(c_cdr);
        return ConstantStruct::get(t.scm_cons, fields);
    }

    template<>
    Constant * ScmCodeGen::initScmConstant<S_FUNC>(vector<Constant*> & fields, int32_t && argc,
                                                   Function *&& fnptr, Function *&& wrfnptr) {
        D(cerr << "constant func" << endl);
        fields.push_back(builder.getInt32((uint32_t)argc));
        assert(fnptr);
        Constant * c_fnptr = ConstantExpr::getCast(Instruction::BitCast, fnptr, t.scm_fn_ptr);
        Constant * c_ctxptr = ConstantPointerNull::get(PointerType::get(t.scm_type_ptr, 0));
        fields.push_back(c_fnptr);
        fields.push_back(wrfnptr);
        fields.push_back(c_ctxptr);
        return ConstantStruct::get(t.scm_func, fields);
    }

    StructType *ScmCodeGen::getScmStrType(Type * strt) {
        vector<Type*> fields = { t.ti32, t.ti32, strt };
        return StructType::get(context, fields);
    }

    /*void ScmCodeGen::addTestFunc() {
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
    }*/

    /*void ScmCodeGen::testAstVisit() {
        Value * code = codegen(ast);
        assert(code == nullptr);
    }*/

    void ScmCodeGen::initExternFuncs() {
        FunctionType * func_type = FunctionType::get(
                t.scm_type_ptr,
                { t.ti32, t.scm_fn_ptr, t.scm_wrfn_ptr,
                  PointerType::get(t.scm_type_ptr, 0) },
                false
        );

        fn.alloc_func = Function::Create(
                func_type,
                GlobalValue::ExternalLinkage,
                RuntimeSymbol::alloc_func, module.get()
        );

        func_type = FunctionType::get(
                PointerType::get(t.scm_type_ptr, 0),
                { t.ti32 },
                false
        );

        fn.alloc_heap_storage = Function::Create(
                func_type,
                GlobalValue::ExternalLinkage,
                RuntimeSymbol::alloc_heap_storage, module.get()
        );

        func_type = FunctionType::get(
                builder.getVoidTy(),
                { t.scm_type_ptr },
                false
        );

        fn.error_not_a_func = Function::Create(
                func_type,
                GlobalValue::ExternalLinkage,
                RuntimeSymbol::error_not_a_func, module.get()
        );

        func_type = FunctionType::get(
                builder.getVoidTy(),
                { PointerType::get(t.scm_func, 0), t.ti32 },
                false
        );

        fn.error_wrong_arg_num = Function::Create(
                func_type,
                GlobalValue::ExternalLinkage,
                RuntimeSymbol::error_wrong_arg_num, module.get()
        );
    }


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

        BasicBlock * bb = BasicBlock::Create(context, "entry", main_func);
        builder.SetInsertPoint(bb);

        g_exit_code = new GlobalVariable(
                *module, builder.getInt32Ty(), false,
                GlobalValue::ExternalLinkage,
                builder.getInt32(0), RuntimeSymbol::exit_code
        );

        g_argv = new GlobalVariable(
                *module, t.scm_type_ptr, false,
                GlobalValue::ExternalLinkage,
                ConstantPointerNull::get(t.scm_type_ptr), RuntimeSymbol::argv
        );

        FunctionType * get_argv_func_type = FunctionType::get(
                t.scm_type_ptr,
                main_args_type,
                false
        );

        Function * get_argv_func = Function::Create(
                get_argv_func_type,
                GlobalValue::ExternalLinkage,
                RuntimeSymbol::get_arg_vec, module.get()
        );

        Value * arg_vec = builder.CreateCall(get_argv_func, { int_argc, ptr_argv });
        builder.CreateStore(arg_vec, g_argv);

        /*LoadInst * exit_c = builder.CreateLoad(g_exit_code);
        builder.CreateRet(exit_c);
        builder.SetInsertPoint(exit_c);*/

        entry_func = main_func;
    }

    void ScmCodeGen::addMainFuncEpilog(Value *) {
        LoadInst * exit_c = builder.CreateLoad(g_exit_code);
        builder.CreateRet(exit_c);
    }

    void ScmCodeGen::addLibInitFuncProlog() {
        FunctionType * libinit_func_type = FunctionType::get(
                builder.getVoidTy(),
                {},
                false
        );
        Function * libinit_func = Function::Create(
                libinit_func_type,
                GlobalValue::InternalLinkage,
                "lib_init", module.get()
        );

        libinit_func->setDoesNotThrow();
        libinit_func->setHasUWTable();

        BasicBlock * bb = BasicBlock::Create(context, "entry", libinit_func);
        builder.SetInsertPoint(bb);

        g_exit_code = new GlobalVariable(
                *module, builder.getInt32Ty(), false,
                GlobalValue::WeakAnyLinkage,
                builder.getInt32((uint32_t)-1), RuntimeSymbol::exit_code
        );

        g_argv = new GlobalVariable(
                *module, t.scm_type_ptr, false,
                GlobalValue::WeakAnyLinkage,
                ConstantPointerNull::get(t.scm_type_ptr), RuntimeSymbol::argv
        );

        // With this trick, we can tell whether the lib was loaded from
        // the compiler (exit_code = -1) or from a scheme executable (exit_code = 0).
        // In case of compiler, we don't execute the lib_init code.
        Value * exit_code = builder.CreateLoad(g_exit_code);
        Value * cond_val = builder.CreateICmpEQ(exit_code, builder.getInt32((uint32_t)-1));

        BasicBlock * then_bb = BasicBlock::Create(context, "then", libinit_func);
        BasicBlock * else_bb = BasicBlock::Create(context, "else");

        builder.CreateCondBr(cond_val, then_bb, else_bb);

        builder.SetInsertPoint(then_bb);
        builder.CreateRetVoid();

        libinit_func->getBasicBlockList().push_back(else_bb);
        builder.SetInsertPoint(else_bb);

        entry_func = libinit_func;

        // Make sure the lib_init function is run on library load:
        // Add it to the llvm.global_ctors array.
        StructType * ctor_field_type = StructType::get(
            context,
            {
                t.ti32,
                PointerType::get(FunctionType::get(
                     builder.getVoidTy(), {}, false
                ), 0),
                builder.getInt8PtrTy(0)
            },
            false
        );

        ArrayType * ctor_array_type = ArrayType::get(ctor_field_type, 1);

        g_ctors = new GlobalVariable(
            *module, ctor_array_type , false,
            GlobalValue::AppendingLinkage,
            ConstantArray::get(
                ctor_array_type,
                {
                    ConstantStruct::get(ctor_field_type, {
                        builder.getInt32(65535),
                        libinit_func,
                        ConstantPointerNull::get(builder.getInt8PtrTy())
                    })
                }
            ),
            "llvm.global_ctors"
        );
    }

    void ScmCodeGen::addLibInitFuncEpilog(Value *) {
        builder.CreateRetVoid();
    }

    void ScmCodeGen::addExprFuncProlog() {
        FunctionType * expr_func_type = FunctionType::get(
                t.scm_type_ptr,
                {},
                false
        );
        Function * expr_func = Function::Create(
                expr_func_type,
                GlobalValue::ExternalLinkage,
                entry_func_name, module.get()
        );

        /*expr_func->setDoesNotThrow();
        expr_func->setHasUWTable();*/

        BasicBlock * bb = BasicBlock::Create(context, "entry", expr_func);
        builder.SetInsertPoint(bb);

        entry_func = expr_func;
    }

    void ScmCodeGen::addExprFuncEpilog(Value * last_val) {
        builder.CreateRet(builder.CreateBitCast(last_val, t.scm_type_ptr));
    }


    any_ptr ScmCodeGen::visit(ScmProg * node) {
        D(cerr << "VISITED ScmProg!" << endl);
        Value * ret = nullptr;

        for (auto & e: *node) {
            //e->printSrc(cerr);
            //cerr << endl;
            ret = codegen(e);
        }
        return ret;
    }

    Value * ScmCodeGen::genGlobalConstant(Constant * c) {
        return new GlobalVariable(
                *module, c->getType(), true,
                GlobalValue::InternalLinkage,
                c
        );
    }

    any_ptr ScmCodeGen::visit(ScmInt * node) {
        D(cerr << "VISITED ScmInt!" << endl);
        Constant * c = getScmConstant<S_INT>(node->val);
        /*return node->IR_val = new GlobalVariable(
                *module, t.scm_int, true,
                GlobalValue::InternalLinkage,
                c, ""
        );*/
        return node->IR_val = genGlobalConstant(c);
    }

    any_ptr ScmCodeGen::visit(ScmFloat * node) {
        D(cerr << "VISITED ScmFloat!" << endl);
        Constant * c = getScmConstant<S_FLOAT>(node->val);
        /*return node->IR_val = new GlobalVariable(
                *module, t.scm_float, true,
                GlobalValue::InternalLinkage,
                c, ""
        );*/
        return node->IR_val = genGlobalConstant(c);
    }

    any_ptr ScmCodeGen::visit(ScmTrue * node) {
        D(cerr << "VISITED ScmTrue!" << endl);
        Constant * c = getScmConstant<S_TRUE>();
        /*return node->IR_val = new GlobalVariable(
                *module, t.scm_type, true,
                GlobalValue::InternalLinkage,
                c, ""
        );*/
        return node->IR_val = genGlobalConstant(c);
    }

    any_ptr ScmCodeGen::visit(ScmFalse * node) {
        D(cerr << "VISITED ScmFalse!" << endl);
        Constant * c = getScmConstant<S_FALSE>();
        /*return node->IR_val = new GlobalVariable(
                *module, t.scm_type, true,
                GlobalValue::InternalLinkage,
                c, ""
        );*/
        return node->IR_val = genGlobalConstant(c);
    }

    any_ptr ScmCodeGen::visit(ScmNull * node) {
        D(cerr << "VISITED ScmNull!" << endl);
        Constant * c = getScmConstant<S_NIL>();
        /*return node->IR_val = new GlobalVariable(
                *module, t.scm_type, true,
                GlobalValue::InternalLinkage,
                c, ""
        );*/
        return node->IR_val = genGlobalConstant(c);
    }

    any_ptr ScmCodeGen::visit(ScmStr * node) {
        D(cerr << "VISITED ScmStr!" << endl);
        // Each string has a different type according to its length.
        // That type must match with the global variable type.
        Constant * c = getScmConstant<S_STR>(node->val);
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
        Constant * c = getScmConstant<S_SYM>(node->val);
        Type * str_type = c->getAggregateElement(2)->getType();
        return node->IR_val = new GlobalVariable(
                *module, getScmStrType(str_type), true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    any_ptr ScmCodeGen::visit(ScmRef * node) {
        D(cerr << "VISITED ScmRef!" << endl);
        // We have to translate different kinds of Refs.
        // Direct access to locals and globals, indirect to closure variables.
        P_ScmObj robj = node->refObj();

        //D(cerr << "location: " << robj->location << endl);

        if (robj->location == T_HEAP_LOC) {
            D(cerr << "num_of_levels_up = " << node->num_of_levels_up << endl);
            // Get function where the referenced object is defined
            ScmFunc * def_func = robj->defined_in_func;
            assert(def_func);
            auto idx_it = def_func->heap_local_idx.find(robj.get());
            assert(idx_it != def_func->heap_local_idx.end());

            // Get context pointer of the current function
            ScmFunc * curr_func = node->defined_in_func;
            assert(curr_func);
            Value * heap_st = curr_func->IR_context_ptr;
            assert(heap_st);

            int i = node->num_of_levels_up - 1;
            while(i--) {
                heap_st = genHeapLoad(heap_st, 0);
                heap_st = builder.CreateBitCast(heap_st, PointerType::get(t.scm_type_ptr, 0));
            }
            D(cerr << "heap_st_idx = " << idx_it->second << endl);
            return genHeapLoad(heap_st, idx_it->second);
        }

        /*if (!robj->IR_val && robj->t == T_FUNC) {
            // First reference to native function
            assert(DPC<ScmFunc>(robj)->body_list == nullptr);
            // We must initialize its IR_val
            codegen(robj);
        }

        // In other cases we always have a reference to something
        // for which the code was already generated.
        // Therefore the missing IR_val is most likely a bug.
        assert(robj->IR_val);*/

        if (robj->t == T_FUNC) {
            ScmFunc * fn_obj = DPC<ScmFunc>(robj).get();
            assert(fn_obj);

            Function * func = dyn_cast<Function>(codegen(robj));
            assert(func);

            assert(fn_obj->IR_wrapper_fn_ptr);

            if (fn_obj->has_closure) {
                ScmFunc * curr_func = node->defined_in_func;

                return genAllocFunc(fn_obj->argc_expected, func,
                                    fn_obj->IR_wrapper_fn_ptr, curr_func->IR_heap_storage);
            }
            else {
                return genConstFunc(fn_obj->argc_expected, func, fn_obj->IR_wrapper_fn_ptr);
            }
        }

        if (robj->location == T_GLOB) {
            Value * ret_val;
            // Handle external globals
            if (robj->is_extern) {
                GlobalVariable * gvar = module->getGlobalVariable(robj->exported_name);
                if (!gvar) {
                    gvar = new GlobalVariable(
                            *module, t.scm_type_ptr, false,
                            GlobalValue::ExternalLinkage,
                            nullptr, robj->exported_name
                    );
                }
                ret_val = gvar;
            }
            else {
                ret_val = robj->IR_val;
            }

            return builder.CreateLoad(ret_val);
        }

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

        Constant * c = getScmConstant<S_CONS>(car, cdr);
        return node->IR_val = new GlobalVariable(
                *module, t.scm_cons, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    Value * ScmCodeGen::genConstFunc(int32_t argc, Function * fnptr, Function * wrfnptr) {
        Constant * c = getScmConstant<S_FUNC>(argc, fnptr, wrfnptr);
        return new GlobalVariable(
                *module, t.scm_func, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    Value * ScmCodeGen::genAllocFunc(int32_t argc, Function * fnptr,
                                     Function * wrfnptr, Value * ctxptr) {
        assert(ctxptr);

        return builder.CreateCall(
                fn.alloc_func,
                { builder.getInt32((uint32_t)argc),
                  builder.CreateBitCast(fnptr, t.scm_fn_ptr),
                  wrfnptr,
                  ctxptr }
        );
    }

    Value * ScmCodeGen::genAllocHeapStorage(int32_t size) {
        // size is the number of objects we need to
        // store on the heap for the current function
        // including the parent heap storage pointer

        return builder.CreateCall(
                fn.alloc_heap_storage,
                { builder.getInt32((uint32_t)size) },
                "__heap_storage"
        );
    }

    void ScmCodeGen::genHeapStore(Value * hs, Value * obj, int32_t idx) {
        Value * hs_idx = builder.CreateGEP(hs, builder.getInt32((uint32_t)idx));
        builder.CreateStore(builder.CreateBitCast(obj, t.scm_type_ptr), hs_idx);
    }

    Value * ScmCodeGen::genHeapLoad(Value * hs, int32_t idx) {
        Value * hs_idx = builder.CreateGEP(hs, builder.getInt32((uint32_t)idx));
        return builder.CreateLoad(hs_idx);
    }

    void ScmCodeGen::declFuncWrapper(ScmFunc * node, GlobalValue::LinkageTypes linkage) {
        // TODO: We don't need to generate wrappers for functions which take no arguments
        Function * func = Function::Create(
                t.scm_wrfn_sig,
                linkage,
                "argl_" + node->name, module.get()
        );

        node->IR_wrapper_fn_ptr = func;
    }

    void ScmCodeGen::defFuncWrapper(ScmFunc * node, Function * func) {
        auto saved_ip = builder.saveIP();

        Function * fn_wrapper = node->IR_wrapper_fn_ptr;
        BasicBlock * bb = BasicBlock::Create(context, "entry", fn_wrapper);
        builder.SetInsertPoint(bb);
        Value * arg_list = fn_wrapper->args().begin();
        vector<Value *> args;
        int i = 0;

        if (node->argc_expected) {
            for (; i < node->argc_expected; i++) {
                Value * arg_idx = builder.CreateGEP(
                        arg_list, builder.getInt32((uint32_t)i)
                );
                Value * arg_val = builder.CreateLoad(arg_idx);
                args.push_back(arg_val);
            }
        }

        if (node->has_closure) {
            // Also pass the context pointer
            Value * arg_idx = builder.CreateGEP(
                    arg_list, builder.getInt32((uint32_t)i)
            );
            Value * arg_val = builder.CreateLoad(arg_idx);
            arg_val = builder.CreateBitCast(arg_val, PointerType::get(t.scm_type_ptr, 0));
            args.push_back(arg_val);
        }
        builder.CreateRet(builder.CreateCall(func, args));
        builder.restoreIP(saved_ip);
    }

    any_ptr ScmCodeGen::visit(ScmFunc * node) {
        D(cerr << "VISITED ScmFunc!" << endl);
        if (node->IR_val) return node->IR_val;

        vector<Type*> arg_types;
        FunctionType * func_type;
        Function * func;
        bool varargs = false;
        auto saved_ip = builder.saveIP();

        Value * heap_storage = nullptr;
        auto & heap_local_idx = node->heap_local_idx;

        if (node->argc_expected == ArgsAnyCount) {
            varargs = true;
            arg_types.push_back(t.scm_type_ptr);
            // TODO: What about closure functions with variable arguments?
            // Of course, so far the parser doesn't support scheme functions
            // with varargs. But when it does, we need to take care of this.
        }
        else {
            arg_types.insert(arg_types.end(), (uint32_t)node->argc_expected, t.scm_type_ptr);
            // Changed the convention to pass context pointer as the last argument.
            // Then we can used it for varargs calls as an argument end mark instead of
            // passing the i32 with argc. That way all the function types will be compatible
            // with this new default type: %scm_type* (%scm_type*, ...)*.
            if (node->has_closure) {
                arg_types.push_back(PointerType::get(t.scm_type_ptr, 0));
            }
        }

        auto linkage = GlobalValue::InternalLinkage;
        if (node->location == T_GLOB && !StringRef(node->name).startswith("__lambda#")) {
            linkage = GlobalValue::ExternalLinkage;
        }

        func_type = FunctionType::get(t.scm_type_ptr, arg_types, varargs);

        func = Function::Create(
                func_type,
                linkage,
                node->name, module.get()
        );

        // Save function declaration
        node->IR_val = func;

        declFuncWrapper(node, linkage);

        if (node->is_extern || !node->arg_list) {
            // Return declaration only (in case of extern functions).
            return func;
        }

        BasicBlock * bb = BasicBlock::Create(context, "entry", func);
        Value * ret_val, * c_ret_val;
        builder.SetInsertPoint(bb);

        // Generate code for heap storage allocation
        if (node->passing_closure || heap_local_idx.size() > 0) {
            heap_storage = node->IR_heap_storage =
                    genAllocHeapStorage((int32_t)heap_local_idx.size() + 1);
        }

        auto arg_it = func->args().begin();

        if (node->argc_expected) {
            DPC<ScmCons>(node->arg_list)->each(
                [this, &arg_it, &heap_local_idx, &heap_storage](P_ScmObj e) {
                    ScmRef *fn_arg_ref = dynamic_cast<ScmRef *>(e.get());
                    assert(fn_arg_ref);
                    P_ScmObj fn_arg = fn_arg_ref->refObj();
                    fn_arg->IR_val = arg_it;

                    // Examine argument location type and copy
                    // the argument to heap storage if necessary.
                    if (fn_arg->location == T_HEAP_LOC) {
                        auto idx_it = heap_local_idx.find(fn_arg.get());
                        assert(idx_it != heap_local_idx.end());

                        int32_t idx = idx_it->second;
                        genHeapStore(heap_storage, fn_arg->IR_val, idx);
                    }

                    arg_it++;
                }
            );
        }

        if (node->has_closure) {
            // Last argument is the context pointer
            node->IR_context_ptr = arg_it++;
            if (node->passing_closure) {
                // Store context pointer to the heap storage at 0th index.
                genHeapStore(heap_storage, node->IR_context_ptr, 0);
            }
        }
        else {
            node->IR_context_ptr = nullptr;
        }

        DPC<ScmCons>(node->body_list)->each([this, &ret_val](P_ScmObj e) {
            ret_val = codegen(e);
        });
        c_ret_val = builder.CreateBitCast(ret_val, t.scm_type_ptr);
        builder.CreateRet(c_ret_val);
        verifyFunction(*func, &errs());

        builder.restoreIP(saved_ip);

        defFuncWrapper(node, func);
        return func;
    }

    vector<Value*> ScmCodeGen::genArgValues(const ScmCall * node) {
        vector<Value*> args;
        if (node->arg_list->t != T_NULL) {
            // At least one argument
            ScmCons * arg_list = DPC<ScmCons>(node->arg_list).get();
            /*if (fn_obj->argc_expected == ArgsAnyCount) {
                args.push_back(builder.getInt32((uint32_t)arg_list->length()));
            }*/

            arg_list->each([this, &args](P_ScmObj e) {
                Value * a = codegen(e);
                if (a->getType() != t.scm_type_ptr) {
                    a = builder.CreateBitCast(a, t.scm_type_ptr);
                }
                args.push_back(a);
            });
        }
        return args;
    }

    any_ptr ScmCodeGen::visit(ScmCall * node) {
        D(cerr << "VISITED ScmCall!" << endl);
        if (node->indirect) {
            // Indirect call of scm_func object (that includes passing
            // closure context pointer). Runtime type check of the called object is needed.
            // In the most general case, obj can be any expression which gives
            // us a scm_func pointer after runtime evaluation.
            Value * obj = codegen(node->fexpr);
            // We must therefore generate a code that will check the type of the referenced object,
            // then it will check the expected and given number of arguments and finally
            // it will call the function poiner or throw a runtime error.
            Value * func = builder.CreateBitCast(obj, PointerType::get(t.scm_func, 0));

            Value * ret = genIfElse(
                    [this, obj] () { // IF the object tag equals S_FUNC
                        D(cerr << "loading tag" << endl);
                        vector<Value*> tag_indices(2, builder.getInt32(0));
                        Value * tag = builder.CreateLoad(
                                t.ti32, builder.CreateGEP(obj, tag_indices)
                        );

                        return builder.CreateICmpEQ(tag, builder.getInt32(S_FUNC));
                    },
                    [this, func, node] () { // obj is FUNC
                        vector<Value*> argc_indices = {
                                builder.getInt32(0),
                                builder.getInt32(1)
                        };
                        Value * argc = builder.CreateLoad(
                                t.ti32, builder.CreateGEP(func, argc_indices)
                        );

                        Value * argc_given = builder.getInt32((uint32_t)node->argc);

                        return genIfElse( // IF the func's number of args equals args_given or ArgsAnyCount
                                [this, func, node, argc, argc_given] () {
                                    D(cerr << "loading argc" << endl);
                                    return builder.CreateOr(
                                            builder.CreateICmpEQ(argc, argc_given),
                                            builder.CreateICmpEQ(argc, builder.getInt32((uint32_t)ArgsAnyCount))
                                    );
                                },
                                [this, func, node] () { // FUNC has the right number of arguments
                                    D(cerr << "prepare to emit indirect call" << endl);
                                    // Emit the indirect call
                                    vector<Value *> fnptr_indices = {
                                            builder.getInt32(0),
                                            builder.getInt32(2)
                                    };
                                    vector<Value *> ctxptr_indices = {
                                            builder.getInt32(0),
                                            builder.getInt32(4)
                                    };

                                    D(cerr << "loading fnptr" << endl);
                                    Value *fnptr = builder.CreateLoad(
                                            t.scm_fn_ptr, builder.CreateGEP(func, fnptr_indices)
                                    );

                                    D(cerr << "loading ctxptr" << endl);
                                    Value *ctxptr = builder.CreateLoad(
                                            PointerType::get(t.scm_type_ptr, 0),
                                            builder.CreateGEP(func, ctxptr_indices)
                                    );

                                    vector<Value *> args = genArgValues(node);
                                    // We have to cast ctxptr to scm_type*
                                    // in case we pass it as the first argument.
                                    ctxptr = builder.CreateBitCast(ctxptr, t.scm_type_ptr);
                                    args.push_back(ctxptr); // This is null for non-closure functions

                                    return builder.CreateCall(t.scm_fn_sig, fnptr, args);
                                },
                                [this, func, argc, argc_given] { // ELSE Error: wrong number of arguments
                                    builder.CreateCall(fn.error_wrong_arg_num, { func, argc_given });
                                    return ConstantPointerNull::get(t.scm_type_ptr);
                                }
                        );
                    },
                    [this, obj] () { // ELSE Error: obj is not FUNC
                        builder.CreateCall(fn.error_not_a_func, { obj });
                        return ConstantPointerNull::get(t.scm_type_ptr);
                    }
            );

            return node->IR_val = ret;
        }
        else {
            ScmRef * fn_ref = DPC<ScmRef>(node->fexpr).get();
            assert(fn_ref);
            ScmFunc * fn_obj = DPC<ScmFunc>(fn_ref->refObj()).get();
            assert(fn_obj);
            // We don't call codegen on the ref. That would yield scm_func struct.
            // Instead we get the raw function pointer from the referenced object.
            Function * func = dyn_cast<Function>(codegen(fn_obj));

            vector<Value*> args = genArgValues(node);

            if (fn_obj->has_closure) {
                // We must also count with the case of direct closure function call.
                // There's no need to allocate scm_func object, we're not passing
                // it around. We just take the heap storage of current function.
                // It must be the current function. Closure with any other context
                // would have to be defined elsewhere and passed as a scm_func struct
                // which would then lead to an indirect call.
                assert(fn_ref->defined_in_func);
                D(cerr << fn_ref->defined_in_func << endl);
                args.push_back(fn_ref->defined_in_func->IR_heap_storage);
            }

            if (fn_obj->argc_expected == ArgsAnyCount) {
                args.push_back(ConstantPointerNull::get(
                        PointerType::get(t.scm_type_ptr, 0)
                ));

                if (args.size() == 1) {
                    args[0] = builder.CreateBitCast(args[0], t.scm_type_ptr);
                }
            }

            return node->IR_val = builder.CreateCall(func, args, fn_obj->name);
        }
    }

    any_ptr ScmCodeGen::visit(ScmDefineVarSyntax * node) {
        D(cerr << "VISITED ScmDefineVarSyntax!" << endl);

        if (node->val->t == T_FUNC && node->val->location == T_GLOB) {
            ScmFunc * fn = DPC<ScmFunc>(node->val).get();
            if (!StringRef(fn->name).startswith("__lambda#")) {
                // Metadata saved only for global named functions
                output_meta.addRecord(fn->argc_expected, fn->name);
            }
        }
        if (node->val->t == T_FUNC || node->val->t == T_REF) {
            codegen(node->val);
            //return codegen(node->val);
            return ConstantPointerNull::get(t.scm_type_ptr);
        }

        Value * expr = codegen(node->val);
        PointerType * etype = dyn_cast<PointerType>(expr->getType());
        assert(etype);

        // We should not always create global variables.
        // That's needed for top-level definitions only.

        if (node->val->location == T_GLOB) {
            ScmSym * defname = DPC<ScmSym>(node->name).get();

            Value * gvar = new GlobalVariable(
                    *module, etype, false,
                    GlobalValue::ExternalLinkage,
                    ConstantPointerNull::get(etype),
                    defname->val
            );

            // Save expr to global var
            builder.CreateStore(expr, gvar);
            node->val->IR_val = gvar;
            node->val->exported_name = defname->val;
        }
        else if (node->val->location == T_HEAP_LOC) {
            // Save expr to heap storage
            ScmFunc * curr_func = node->defined_in_func;
            assert(curr_func);

            auto idx_it = curr_func->heap_local_idx.find(node->val.get());
            assert(idx_it != curr_func->heap_local_idx.end());

            int32_t idx = idx_it->second;
            genHeapStore(curr_func->IR_heap_storage, node->val->IR_val, idx);
        }

        return ConstantPointerNull::get(t.scm_type_ptr);
    }

    any_ptr ScmCodeGen::visit(ScmIfSyntax * node) {
        D(cerr << "VISITED ScmIfSyntax!" << endl);
        Value * cond = codegen(node->cond_expr);
        vector<Value*> indices(2, builder.getInt32(0));
        Value * cond_arg_addr = builder.CreateGEP(cond, indices);
        Value * cond_tag = builder.CreateLoad(cond_arg_addr);
        cond = builder.CreateICmpNE(cond_tag, builder.getInt32(S_FALSE));

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
            DPC<ScmCons>(node->bind_list)->each([this, node](P_ScmObj e) {
                assert(e->t == T_CONS);
                shared_ptr<ScmCons> kv = DPC<ScmCons>(e);
                assert(kv->cdr->t == T_CONS);
                P_ScmObj & expr = DPC<ScmCons>(kv->cdr)->car;
                Value * expr_val = codegen(expr);
                PointerType * etype = dyn_cast<PointerType>(expr_val->getType());

                if (expr->location == T_GLOB) {
                    Value * gvar = new GlobalVariable(
                            *module, etype, false,
                            GlobalValue::InternalLinkage,
                            ConstantPointerNull::get(etype), ""
                    );

                    // Save expr to global var
                    builder.CreateStore(expr_val, gvar);
                    expr->IR_val = gvar;
                }
                else if (expr->location == T_HEAP_LOC) {
                    // Save expr to heap storage
                    ScmFunc * curr_func = node->defined_in_func;
                    assert(curr_func);

                    auto idx_it = curr_func->heap_local_idx.find(expr.get());
                    assert(idx_it != curr_func->heap_local_idx.end());

                    int32_t idx = idx_it->second;
                    genHeapStore(curr_func->IR_heap_storage, expr_val, idx);
                }

                // In case of top level let expressions the bound variables are all
                // actually global.
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

    Value * ScmCodeGen::genAndExpr(ScmCons * cell) {
        if (cell->cdr->t == T_NULL) {
            // Last expression
            Value * expr = codegen(cell->car);
            return builder.CreateBitCast(expr, t.scm_type_ptr);
        }
        return genIfElse(
                [this, cell] () {
                    Value * expr = codegen(cell->car);
                    vector<Value*> indices(2, builder.getInt32(0));
                    Value * expr_arg_addr = builder.CreateGEP(expr, indices);
                    Value * expr_tag = builder.CreateLoad(expr_arg_addr);
                    return builder.CreateICmpEQ(expr_tag, builder.getInt32(S_FALSE));
                },
                [this] () {
                    Constant * c = getScmConstant<S_FALSE>();
                    return genGlobalConstant(c);
                },
                [this, cell] () {
                    return genAndExpr(DPC<ScmCons>(cell->cdr).get());
                }
        );
    }

    any_ptr ScmCodeGen::visit(ScmAndSyntax * node) {
        D(cerr << "VISITED ScmAndSyntax!" << endl);
        ScmCons * expr_list = DPC<ScmCons>(node->expr_list).get();
        if (!expr_list) {
            Constant * c = getScmConstant<S_TRUE>();
            return node->IR_val = genGlobalConstant(c);
        }

        return genAndExpr(expr_list);
    }

    Value * ScmCodeGen::genOrExpr(ScmCons * cell) {
        if (cell->cdr->t == T_NULL) {
            // Last expression
            Value * expr = codegen(cell->car);
            return builder.CreateBitCast(expr, t.scm_type_ptr);
        }

        Value * expr = codegen(cell->car);
        return genIfElse(
                [this, cell, expr] () {
                    vector<Value*> indices(2, builder.getInt32(0));
                    Value * expr_arg_addr = builder.CreateGEP(expr, indices);
                    Value * expr_tag = builder.CreateLoad(expr_arg_addr);
                    return builder.CreateICmpNE(expr_tag, builder.getInt32(S_FALSE));
                },
                [this, expr] () {
                    return builder.CreateBitCast(expr, t.scm_type_ptr);
                },
                [this, cell] () {
                    return genOrExpr(DPC<ScmCons>(cell->cdr).get());
                }
        );
    }

    any_ptr ScmCodeGen::visit(ScmOrSyntax * node) {
        D(cerr << "VISITED ScmOrSyntax!" << endl);
        ScmCons * expr_list = DPC<ScmCons>(node->expr_list).get();
        if (!expr_list) {
            Constant * c = getScmConstant<S_FALSE>();
            return node->IR_val = genGlobalConstant(c);
        }

        return genOrExpr(expr_list);
    }

    void ScmCodeGen::run() {
        Value * last_val;

        (this->*addEntryFuncProlog)();
        last_val = codegen(ast);
        (this->*addEntryFuncEpilog)(last_val);
        verifyFunction(*entry_func, &errs());

        // Save generated metadata array to global variable
        Constant * llsmeta = ConstantDataArray::get(context, output_meta.getBlob());

        new GlobalVariable(
                *module, llsmeta->getType(), false,
                GlobalValue::AppendingLinkage,
                llsmeta, "__llscheme_metainfo__"
        );
    }
}

