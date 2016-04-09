#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cinttypes>
#include <vector>
#include <llvm/ADT/STLExtras.h>
#include "../../include/runtime.h"
#include "../../include/runtime/internal.hpp"
#include "../../include/reader.hpp"
#include "../../include/parser.hpp"
#include "../../include/environment.hpp"
#include "../../include/codegen.hpp"

#define EPSILON 10E-9

namespace llscm {
    namespace runtime {
        using namespace llvm;
        using namespace orc;

        InitJIT::InitJIT() {
            InitializeNativeTarget();
            InitializeNativeTargetAsmPrinter();
            InitializeNativeTargetAsmParser();
            jit = make_unique<ScmJIT>();
            D(std::cerr << "InitJIT completed" << std::endl);
        }

        ScmJIT * InitJIT::getJIT() {
            return jit.get();
        }

        static string getUniqID(const string & name) {
            static ScmNameGen gen;
            return gen.getUniqID(name);
        }

        static ScmJIT * getJIT() {
            static InitJIT jit_obj;
            return jit_obj.getJIT();
        }

        LibSetup::~LibSetup() {
           mem_cleanup();
        }

        static LibSetup lib_setup;


        scm_type_t Constant::scm_null = { S_NIL };
        scm_type_t Constant::scm_true = { S_TRUE };
        scm_type_t Constant::scm_false = { S_FALSE };

#define SCM_VARARGS_WRAPPER(func) \
        scm_type_t * func(scm_type_t * arg0, ...) { \
            va_list ap; \
            va_start(ap, arg0); \
\
            scm_type_t * res = internal_##func( \
                    [&arg0] () { return arg0; }, \
                    [&ap] () { return va_arg(ap, scm_type_t*); } \
            ); \
\
            va_end(ap); \
            return res; \
        }

#define SCM_VARGLIST_WRAPPER(func) \
        scm_type_t * argl_##func(scm_type_t ** arg_list) { \
            int32_t idx = 1; \
            return internal_##func( \
                [arg_list] () { return arg_list[0]; }, \
                [arg_list, &idx] () { return arg_list[idx++]; } \
            ); \
        }

#define SCM_VA_WRAPPERS(func) \
        SCM_VARARGS_WRAPPER(func) \
        SCM_VARGLIST_WRAPPER(func)

        // Template functions cannot have C linkage, so we need to save
        // pointers to their instantiated variants.
#define SCM_ARGLIST_WRAPPER(func) \
        scm_type_t * argl_##func(scm_type_t ** arg_list) { \
            return Argc<arity(func)>::argl_wrapper<decltype(func), &func>(arg_list); \
        }

#define DEF_WITH_WRAPPER(func, ...) \
        SCM_ARGLIST_WRAPPER(func); \
        scm_type_t * func(__VA_ARGS__) \

        SCM_VA_WRAPPERS(scm_plus);
        SCM_VA_WRAPPERS(scm_minus);
        SCM_VA_WRAPPERS(scm_times);
        SCM_VA_WRAPPERS(scm_div);

        // DEF_WITH_WRAPPER expands to:
        // auto argl_scm_display = SCM_ARGLIST_WRAPPER(scm_display);
        // scm_type_t * scm_display(scm_ptr_t obj) {
        DEF_WITH_WRAPPER(scm_display, scm_ptr_t obj) {
            switch (obj->tag) {
                case S_STR:
                    printf("%s", obj.asStr->str);
                    break;
                case S_SYM:
                    printf("%s", obj.asSym->sym);
                    break;
                case S_INT:
                    printf("%" PRId64, obj.asInt->value);
                    break;
                case S_FLOAT:
                    printf("%g", obj.asFloat->value);
                    break;
                case S_TRUE:
                    printf("#t");
                    break;
                case S_FALSE:
                    printf("#f");
                    break;
                case S_NIL:
                    printf("()"); // TODO: '()
                    break;
                case S_CONS: {
                    printf("("); // TODO: we should quote the top level list
                    list_foreach(obj.asCons, [](scm_ptr_t elem) {
                        scm_display(elem.asCons->car);
                        if (elem.asCons->cdr->tag == S_CONS) {
                            printf(" ");
                        }
                    });
                    printf(")");
                    break;
                }
                // TODO:
                /*case S_FUNC:
                case S_VEC:*/
                default:
                    INVALID_ARG_TYPE();
            }

            return SCM_NULL;
        }

        //scm_type_t * scm_gt(scm_ptr_t a, scm_ptr_t b) {
        DEF_WITH_WRAPPER(scm_gt, scm_ptr_t a, scm_ptr_t b) {
            // TODO: implement
            return nullptr;
        }

        DEF_WITH_WRAPPER(scm_num_eq, scm_ptr_t a, scm_ptr_t b) {
            if (a->tag == S_INT) {
                if (b->tag == S_INT) {
                    return a.asInt->value == b.asInt->value ? SCM_TRUE : SCM_FALSE;
                }
                if (b->tag == S_FLOAT) {
                    return fabs(a.asInt->value - b.asFloat->value) < EPSILON ? SCM_TRUE : SCM_FALSE;
                }
                INVALID_ARG_TYPE();
            }
            if (a->tag == S_FLOAT) {
                if (b->tag == S_INT) {
                    return fabs(a.asFloat->value - b.asInt->value) < EPSILON ? SCM_TRUE : SCM_FALSE;
                }
                if (b->tag == S_FLOAT) {
                    return fabs(a.asFloat->value - b.asFloat->value) < EPSILON ? SCM_TRUE : SCM_FALSE;
                }
                INVALID_ARG_TYPE();
            }
            return SCM_NULL;
        }

        DEF_WITH_WRAPPER(scm_cons, scm_ptr_t car, scm_ptr_t cdr) {
            return alloc_cons(car, cdr);
        }

        DEF_WITH_WRAPPER(scm_car, scm_ptr_t obj) {
            if (obj->tag != S_CONS) {
                INVALID_ARG_TYPE();
            }

            return obj.asCons->car;
        }

        DEF_WITH_WRAPPER(scm_cdr, scm_ptr_t obj) {
            if (obj->tag != S_CONS) {
                INVALID_ARG_TYPE();
            }

            return obj.asCons->cdr;
        }

        DEF_WITH_WRAPPER(scm_is_null, scm_ptr_t obj) {
            // TODO: make sure SCM_NULL is a singleton so we can compare pointers only
            return obj->tag == S_NIL ? SCM_TRUE : SCM_FALSE;
        }

        // Used only internally - no need for a wrapper
        scm_type_t * scm_get_arg_vector(int argc, char * argv[]) {
            scm_ptr_t obj = alloc_vec(argc - 1);
            for (int i = 1; i < argc; i++) {
                obj.asVec->elems[i - 1] = alloc_str(argv[i]);
            }
            return obj;
        }

        DEF_WITH_WRAPPER(scm_vector_length, scm_ptr_t obj) {
            if (obj->tag != S_VEC) {
                INVALID_ARG_TYPE();
            }

            scm_ptr_t len = alloc_int(obj.asVec->size);
            return len;
        }

        DEF_WITH_WRAPPER(scm_vector_ref, scm_ptr_t obj, scm_ptr_t idx) {
            if (obj->tag != S_VEC) {
                INVALID_ARG_TYPE();
            }

            if (idx->tag != S_INT) {
                INVALID_ARG_TYPE();
            }

            return obj.asVec->elems[idx.asInt->value];
        }

        DEF_WITH_WRAPPER(scm_cmd_args) {
            return scm_argv;
        }

        DEF_WITH_WRAPPER(scm_length, scm_ptr_t list) {
            if (list->tag == S_NIL) {
                return alloc_int(0);
            }

            if (list->tag != S_CONS) {
                INVALID_ARG_TYPE();
            }

            int64_t len = 0;
            list_foreach(list, [&len](scm_ptr_t elem) {
                len++;
            });

            return alloc_int(len);
        }

        DEF_WITH_WRAPPER(scm_apply, scm_ptr_t func, scm_ptr_t list) {
            if (func->tag != S_FUNC) {
                INVALID_ARG_TYPE();
            }

            bool args_any_count = func.asFunc->argc == -1;

            if (list->tag == S_NIL && (func.asFunc->argc == 0 || args_any_count)) {
                // Call a function without arguments
                // with an optional context pointer (or null).
                // Extra argument is fine because we use
                // the C calling convention. That means the caller
                // cleans up arguments from stack.
                return func.asFunc->fnptr((scm_type_t*)func.asFunc->ctxptr);
            }

            if (list->tag != S_CONS) {
                INVALID_ARG_TYPE();
            }

            int64_t argc = 0;
            vector<scm_type_t*> arg_vec;

            list_foreach(list, [&arg_vec, &argc](scm_ptr_t elem) {
                arg_vec.push_back(elem.asCons->car);
                argc++;
            });

            if (!args_any_count && argc != func.asFunc->argc) {
                error_wrong_arg_num(func.asFunc, argc);
            }
            arg_vec.push_back((scm_type_t*)func.asFunc->ctxptr);

            return func.asFunc->wrfnptr(&arg_vec[0]);
        }

        DEF_WITH_WRAPPER(scm_make_base_nspace) {
            GCed<ScmEnv> * env = new GCed<ScmEnv>(nullptr);
            initGlobalEnvironment(env);

            return alloc_nspace(env);
        }

        DEF_WITH_WRAPPER(scm_eval, scm_ptr_t expr, scm_ptr_t ns) {
            if (ns->tag != S_NSPACE) {
                INVALID_ARG_TYPE();
            }

            unique_ptr<Reader> r = make_unique<ListReader>(expr);
            unique_ptr<Parser> p = make_unique<Parser>(r);

            ScmProg prog = p->NT_Prog();

            if (p->fail()) {
                EVAL_FAILED();
            }

            //shared_ptr<ScmEnv> env = createGlobalEnvironment(prog);
            P_ScmEnv env = ns.asNspace->env->getSharedPtr();
            env->setProg(prog);

            if (!prog.CT_Eval(env)) {
                EVAL_FAILED();
            }

            /*for (auto & e: prog) {
                e->printSrc(cerr);
                cerr << endl;
            }*/

            string expr_name = getUniqID("__anon_expr#");

            ScmCodeGen cg(getGlobalContext(), &prog);
            cg.makeExpression(expr_name);
            cg.run();
            cg.dump();

            // Compile the Module
            ScmJIT * jit = getJIT();
            shared_ptr<Module> mod = cg.getModule();
            mod->setDataLayout(jit->getTargetMachine().createDataLayout());

            // TODO: optimizations (PassManager)
            jit->addModule(mod);
            JITSymbol expr_func_symbol = jit->findSymbol(expr_name);
            assert(expr_func_symbol);

            // Prepare the environment for any future compilation
            env->setGlobalsAsExternal();

            scm_expr_ptr_t expr_func = (scm_expr_ptr_t)expr_func_symbol.getAddress();

            // Call the compiled function
            return expr_func();
        }
    }
}

