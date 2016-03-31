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
    const char * RuntimeSymbol::get_arg_vec = "scm_get_arg_vector";
    const char * RuntimeSymbol::argv = "scm_argv";
    const char * RuntimeSymbol::exit_code = "exit_code";
    const char * RuntimeSymbol::alloc_heap_storage = "alloc_heap_storage";
    const char * RuntimeSymbol::alloc_func = "alloc_func";

    ScmCodeGen::ScmCodeGen(LLVMContext &ctxt, ScmProg * tree):
            context(ctxt), builder(ctxt), ast(tree) {
        module = make_shared<Module>("scm_module", context);
        initTypes();
        initExternFuncs();
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

        // %scm_func = type { i32, i32, %scm_type* (%scm_type*, ...)*, %scm_type** }
        // Every function won't be varargs. This is just the default.
        t.scm_fn_sig = FunctionType::get(t.scm_type_ptr, { t.scm_type_ptr }, true);
        t.scm_fn_ptr = PointerType::get(t.scm_fn_sig, 0);
        t.scm_func = module->getTypeByName("scm_func");
        if (!t.scm_func) {
            t.scm_func = StructType::create(context, "scm_func");
            fields = { t.ti32, t.ti32, t.scm_fn_ptr, PointerType::get(t.scm_type_ptr, 0) };
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

    template<>
    Constant * ScmCodeGen::initScmConstant<ScmCodeGen::FUNC>(vector<Constant*> & fields, int32_t && argc, Function *&& fnptr) {
        D(cerr << "constant func" << endl);
        fields.push_back(builder.getInt32((uint32_t)argc));
        assert(fnptr);
        Constant * c_fnptr = ConstantExpr::getCast(Instruction::BitCast, fnptr, t.scm_fn_ptr);
        Constant * c_ctxptr = ConstantPointerNull::get(PointerType::get(t.scm_type_ptr, 0));
        fields.push_back(c_fnptr);
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
                { t.ti32, t.scm_fn_ptr, PointerType::get(t.scm_type_ptr, 0) },
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

        if (!robj->IR_val && robj->t == T_FUNC) {
            // Reference to native function
            assert(DPC<ScmFunc>(robj)->body_list == nullptr);
            return node->IR_val = codegen(robj);
        }

        // In other cases we always have a reference to something
        // for which the code was already generated.
        // Therefore the missing IR_val is most likely a bug.
        assert(robj->IR_val);

        if (robj->t == T_FUNC) {
            ScmFunc * fn_obj = DPC<ScmFunc>(robj).get();
            assert(fn_obj);
            Function * func = dyn_cast<Function>(codegen(robj));
            assert(func);
            if (fn_obj->has_closure) {
                ScmFunc * curr_func = node->defined_in_func;
                return genAllocFunc(fn_obj->argc_expected, func, curr_func->IR_heap_storage);
            }
            else {
                return genConstFunc(fn_obj->argc_expected, func);
            }
        }

        if (robj->location == T_GLOB) {
            return builder.CreateLoad(robj->IR_val);
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

        Constant * c = getScmConstant<CONS>(car, cdr);
        return node->IR_val = new GlobalVariable(
                *module, t.scm_cons, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    Value * ScmCodeGen::genConstFunc(int32_t argc, Function * fnptr) {
        Constant * c = getScmConstant<FUNC>(argc, fnptr);
        return new GlobalVariable(
                *module, t.scm_func, true,
                GlobalValue::InternalLinkage,
                c, ""
        );
    }

    Value * ScmCodeGen::genAllocFunc(int32_t argc, Function * fnptr, Value * ctxptr) {
        assert(ctxptr);

        return builder.CreateCall(
                fn.alloc_func,
                { builder.getInt32((uint32_t)argc),
                  builder.CreateBitCast(fnptr, t.scm_fn_ptr),
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

    any_ptr ScmCodeGen::visit(ScmFunc * node) {
        D(cerr << "VISITED ScmFunc!" << endl);
        if (node->IR_val) return node->IR_val;

        vector<Type*> arg_types;
        FunctionType * func_type;
        Function * func;
        bool varargs = false;
        auto saved_ip = builder.saveIP();

        //Value * context_ptr = nullptr;
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

        func_type = FunctionType::get(t.scm_type_ptr, arg_types, varargs);
        // TODO: How to handle redefinitions?
        func = Function::Create(
                func_type,
                GlobalValue::ExternalLinkage,
                node->name, module.get()
        );

        // Save function declaration
        node->IR_val = func;

        if (!node->arg_list) {
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
            Value * ir_false = builder.getInt1(false);
            // TODO: Implement indirect call of scm_func object (that includes passing
            // closure context pointer). Runtime type check of the called object is needed.
            // In the most general case, obj can be any expression which gives
            // us a scm_func pointer after runtime evaluation.
            Value * obj = codegen(node->fexpr);
            // We must therefore generate a code that will check the type of the referenced object,
            // then it will check the expected and given number of arguments and finally
            // it will call the function poiner or throw a runtime error.

            // TODO: This is only for testing. REMOVE!
            obj = builder.CreateBitCast(obj, PointerType::get(t.scm_func, 0));

            Value * ret = genIfElse(
                    [this, &obj, node, ir_false] () {
                        D(cerr << "loading tag" << endl);
                        vector<Value*> tag_indices(2, builder.getInt32(0));
                        Value * tag = builder.CreateLoad(
                                t.ti32, builder.CreateGEP(obj, tag_indices)
                        );
                        //assert(tag->getType() == t.ti32);

                        /*return genIfElse(
                                // If the given object is FUNC
                                [this, tag] () { return builder.CreateICmpEQ(tag, builder.getInt32(FUNC)); },
                                // And if FUNC's argc equals this call's argc
                                [this, &obj, node] () {
                                    //D(obj->getType()->dump());
                                    //D(PointerType::get(t.scm_func, 0)->dump());
                                    obj = builder.CreateBitCast(obj, PointerType::get(t.scm_func, 0));
                                    D(cerr << "loading argc" << endl);
                                    vector<Value*> argc_indices = {
                                            builder.getInt32(0),
                                            builder.getInt32(1)
                                    };
                                    Value * argc = builder.CreateLoad(
                                            t.ti32, builder.CreateGEP(obj, argc_indices)
                                    );
                                    assert(argc->getType() == t.ti32);
                                    return builder.CreateICmpEQ(argc, builder.getInt32((uint32_t)node->argc));
                                },
                                [this, ir_false] () { return ir_false; }
                        );*/
                        return builder.getInt1(true);
                    },
                    [this, &obj, node] () {
                        D(cerr << "prepare to emit indirect call" << endl);
                        // Emit the indirect call
                        vector<Value*> fnptr_indices = {
                                builder.getInt32(0),
                                builder.getInt32(2)
                        };
                        vector<Value*> ctxptr_indices = {
                                builder.getInt32(0),
                                builder.getInt32(3)
                        };

                        D(cerr << "loading fnptr" << endl);
                        Value * fnptr = builder.CreateLoad(
                                t.scm_fn_ptr, builder.CreateGEP(obj, fnptr_indices)
                        );

                        D(cerr << "loading ctxptr" << endl);
                        Value * ctxptr = builder.CreateLoad(
                                PointerType::get(t.scm_type_ptr, 0),
                                builder.CreateGEP(obj, ctxptr_indices)
                        );

                        vector<Value*> args = genArgValues(node);
                        args.push_back(ctxptr); // This is null for non-closure functions

                        return builder.CreateCall(t.scm_fn_sig, fnptr, args);
                    },
                    [this] () {
                        // TODO: Emit invalid func runtime error
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

        // We should not always create global variables.
        // That's needed for top-level definitions only.

        if (node->val->location == T_GLOB) {
            Value * gvar = new GlobalVariable(
                    *module, etype, false,
                    GlobalValue::InternalLinkage,
                    ConstantPointerNull::get(etype), ""
            );

            // Save expr to global var
            builder.CreateStore(expr, gvar);
            node->val->IR_val = gvar;
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

        return node->val->IR_val;
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

}

