#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <cinttypes>
#include <vector>
#include <iostream>
#include <llvm/ADT/STLExtras.h>
#include "../../include/runtime.h"
#include "../../include/runtime/internal.hpp"
#include "../../include/runtime/readlinestream.hpp"
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

        static readlinestream & getReadlineStream() {
            static readlinestream readlns;
            return readlns;
        }

        LibSetup::~LibSetup() {
           mem_cleanup();
        }

        static LibSetup lib_setup;


        scm_type_t Constant::scm_null = { S_NIL };
        scm_type_t Constant::scm_true = { S_TRUE };
        scm_type_t Constant::scm_false = { S_FALSE };
        scm_type_t Constant::scm_eof = { S_EOF };

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
        SCM_VA_WRAPPERS(scm_list);
        SCM_VA_WRAPPERS(scm_current_nspace);

        // DEF_WITH_WRAPPER expands to:
        // auto argl_scm_display = SCM_ARGLIST_WRAPPER(scm_display);
        // scm_type_t * scm_display(scm_ptr_t obj) {
        DEF_WITH_WRAPPER(scm_display, scm_ptr_t obj) {
            if (!obj.asType) {
                return SCM_NULL;
            }

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
                case S_FUNC: {
                    // TODO: we should store name in scm_func_t
                    printf("#<procedure>");
                    break;
                }
                case S_NSPACE: {
                    printf("#<namespace>");
                    break;
                }
                // TODO:
                /*case S_VEC:*/
                default:
                    INVALID_ARG_TYPE();
            }

            return SCM_NULL;
        }

        //scm_type_t * scm_gt(scm_ptr_t a, scm_ptr_t b) {
        DEF_WITH_WRAPPER(scm_gt, scm_ptr_t a, scm_ptr_t b) {
            if (a->tag == S_INT) {
                if (b->tag == S_INT) {
                    return a.asInt->value > b.asInt->value ? SCM_TRUE : SCM_FALSE;
                }
                if (b->tag == S_FLOAT) {
                    return (a.asInt->value - b.asFloat->value) > EPSILON ? SCM_TRUE : SCM_FALSE;
                }
                INVALID_ARG_TYPE();
            }
            if (a->tag == S_FLOAT) {
                if (b->tag == S_INT) {
                    return (a.asFloat->value - b.asInt->value) > EPSILON ? SCM_TRUE : SCM_FALSE;
                }
                if (b->tag == S_FLOAT) {
                    return (a.asFloat->value - b.asFloat->value) > EPSILON ? SCM_TRUE : SCM_FALSE;
                }
                INVALID_ARG_TYPE();
            }
            INVALID_ARG_TYPE();
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
            INVALID_ARG_TYPE();
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

        // TODO: Move somewhere else?
        static scm_type_t * read_atom(const unique_ptr<Reader> & r) {
            const Token * tok = r->currToken();

            switch (tok->t) {
                case STR:
                    return alloc_str(tok->name.c_str());
                case SYM:
                    return alloc_sym(tok->name.c_str());
                case INT:
                    return alloc_int(tok->int_val);
                case FLOAT:
                    return alloc_float(tok->float_val);
                case ERR:
                    READ_FAILED();
                default:;
            }

            if (tok->t != KWRD) {
                fprintf(stderr, "Invalid token for an atom.\n");
                READ_FAILED();
            }
            switch (tok->kw) {
                case KW_TRUE:
                    return SCM_TRUE;
                case KW_FALSE:
                    return SCM_FALSE;
                case KW_NULL:
                    return SCM_NULL;
                case KW_RPAR:
                    fprintf(stderr, "Unexpected \")\".\n");
                    READ_FAILED();
                default:
                    return alloc_sym(tok->name.c_str());
            }
        }

        static scm_type_t * read_expr(const unique_ptr<Reader> & r);

        static scm_type_t * read_list(const unique_ptr<Reader> & r) {
            const Token * tok = r->currToken();

            if (tok->t == KWRD && tok->kw == KW_RPAR) {
                return SCM_NULL;
            }

            scm_type_t * car = read_expr(r);
            r->nextToken();
            scm_type_t * cdr = read_list(r);

            return alloc_cons(car, cdr);
        }

        static scm_type_t * read_expr(const unique_ptr<Reader> & r) {
            const Token * tok = r->currToken();
            scm_type_t * expr;

            if (!tok) {
                return SCM_EOF;
            }

            if (tok->t == KWRD && tok->kw == KW_LPAR) {
                r->nextToken();
                expr = read_list(r);

                tok = r->currToken();
                if (tok->t != KWRD || tok->kw != KW_RPAR) {
                    fprintf(stderr, "Expected token \")\".\n");
                    READ_FAILED();
                }
            }
            else if(tok->t == KWRD && tok->kw == KW_QUCHAR) {
                r->nextToken();

                scm_type_t * quoted = alloc_cons(read_expr(r), SCM_NULL);
                expr = alloc_cons(alloc_sym("quote"), quoted);
            }
            else {
                expr = read_atom(r);
            }

            return expr;
        }

        DEF_WITH_WRAPPER(scm_read) {
            readlinestream & readlns = getReadlineStream();
            readlns.setPrompt("> ");
            unique_ptr<Reader> r = make_unique<FileReader>(readlns);

            r->nextToken();
            return read_expr(r);
        }

        DEF_WITH_WRAPPER(scm_is_eof, scm_ptr_t obj) {
            return obj->tag == S_EOF ? SCM_TRUE : SCM_FALSE;
        }

        DEF_WITH_WRAPPER(scm_string_to_symbol, scm_ptr_t obj) {
            if (obj->tag != S_STR) {
                INVALID_ARG_TYPE();
            }

            return alloc_sym(obj.asStr->str);
        }

        DEF_WITH_WRAPPER(scm_string_equals, scm_ptr_t a, scm_ptr_t b) {
            if (a->tag != S_STR || b->tag != S_STR) {
                INVALID_ARG_TYPE();
            }
            return !strcmp(a.asStr->str, b.asStr->str) ? SCM_TRUE : SCM_FALSE;
        }

        DEF_WITH_WRAPPER(scm_string_append, scm_ptr_t a, scm_ptr_t b) {
            if (a->tag != S_STR || b->tag != S_STR) {
                INVALID_ARG_TYPE();
            }
            string concat = a.asStr->str;
            concat += b.asStr->str;
            return alloc_str(concat.c_str());
        }

        DEF_WITH_WRAPPER(scm_string_replace, scm_ptr_t str, scm_ptr_t a, scm_ptr_t b) {
            if (str->tag != S_STR || a->tag != S_STR || b->tag != S_STR) {
                INVALID_ARG_TYPE();
            }

            string sstr = str.asStr->str;
            string sa = a.asStr->str;
            string sb = b.asStr->str;

            size_t pos = 0;
            while ((pos = sstr.find(sa, pos)) != string::npos) {
                sstr.replace(pos, sa.length(), sb);
                pos += sb.length();
            }

            return alloc_str(sstr.c_str());
        }

        DEF_WITH_WRAPPER(scm_string_split, scm_ptr_t str) {
            if (str->tag != S_STR) {
                INVALID_ARG_TYPE();
            }

            vector<scm_type_t*> parts;
            string word;
            istringstream ss(str.asStr->str);

            while (ss >> word) {
                parts.push_back(alloc_str(word.c_str()));
            }
            parts.push_back(nullptr);

            return argl_scm_list(&parts[0]);
        }
    }
}

