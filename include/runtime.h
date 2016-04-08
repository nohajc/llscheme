#ifndef LLSCHEME_RUNTIME_H
#define LLSCHEME_RUNTIME_H

#include <memory>
#include <set>
#include "runtime/types.hpp"
#include "runtime/scmjit.hpp"
#include "gc_cpp.h"

#define SCM_NULL &(Constant::scm_null)
#define SCM_TRUE &(Constant::scm_true)
#define SCM_FALSE &(Constant::scm_false)

namespace llscm {
    class ScmEnv;

    namespace runtime {

        class LibSetup {
        public:
            ~LibSetup();
        };

        class InitJIT {
            std::unique_ptr<llvm::orc::ScmJIT> jit;
        public:
            InitJIT();
            llvm::orc::ScmJIT * getJIT();
        };

        struct scm_type_t {
            int32_t tag;
        };

        struct scm_int_t {
            int32_t tag;
            int64_t value;
        };

        struct scm_float_t {
            int32_t tag;
            double value;
        };

        struct scm_str_t {
            int32_t tag;
            int32_t len;
            char str[1];
        };

        struct scm_sym_t {
            int32_t tag;
            int32_t len;
            char sym[1];
        };

        struct scm_cons_t {
            int32_t tag;
            scm_type_t * car;
            scm_type_t * cdr;
        };

        typedef scm_type_t * (*scm_fnptr_t)(scm_type_t * arg0, ...);
        typedef scm_type_t * (*al_wrapper_t)(scm_type_t **);
        typedef scm_type_t * (*scm_expr_ptr_t)();

        struct scm_func_t {
            int32_t tag;
            int32_t argc;
            scm_fnptr_t fnptr;
            al_wrapper_t wrfnptr;
            scm_type_t ** ctxptr;
        };

        struct scm_vec_t {
            int32_t tag;
            int32_t size;
            scm_type_t * elems[1];
        };

        struct scm_nspace_t {
            int32_t tag;
            ScmEnv * env;
        };

        struct Constant {
            static scm_type_t scm_null;
            static scm_type_t scm_true;
            static scm_type_t scm_false;
        };

        // Smart tagged union with convenient operator overloads
        union scm_ptr_t {
            scm_type_t * asType;
            scm_int_t * asInt;
            scm_float_t * asFloat;
            scm_str_t * asStr;
            scm_sym_t * asSym;
            scm_cons_t * asCons;
            scm_func_t * asFunc;
            scm_vec_t * asVec;
            scm_nspace_t * asNspace;

            scm_type_t * operator->() {
                return asType;
            }

            operator scm_type_t*() {
                return asType;
            }

            scm_ptr_t() {
                asType = &Constant::scm_null;
            }

            template<class T>
            scm_ptr_t(T * ptr) {
                asType = (scm_type_t*)ptr;
            }

            template<class T>
            scm_ptr_t operator=(T * ptr) {
                asType = (scm_type_t*)ptr;
                return *this;
            }
        };

#define DECL_WITH_WRAPPER(func, ...) \
        scm_type_t * func(__VA_ARGS__); \
        scm_type_t * argl_##func(scm_type_t ** arg_list)

        extern "C" {
            extern int32_t exit_code;
            extern scm_type_t * scm_argv;

            scm_type_t * scm_get_arg_vector(int argc, char * argv[]);
            //scm_type_t * scm_cmd_args();
            DECL_WITH_WRAPPER(scm_cmd_args);
            // scm_type_t * scm_display(scm_ptr_t obj);
            DECL_WITH_WRAPPER(scm_display, scm_ptr_t obj);
            //scm_type_t * scm_gt(scm_ptr_t a, scm_ptr_t b);
            DECL_WITH_WRAPPER(scm_gt, scm_ptr_t a, scm_ptr_t b);
            //scm_type_t * scm_num_eq(scm_ptr_t a, scm_ptr_t b);
            DECL_WITH_WRAPPER(scm_num_eq, scm_ptr_t a, scm_ptr_t b);
            //scm_type_t * scm_cons(scm_ptr_t car, scm_ptr_t cdr);
            DECL_WITH_WRAPPER(scm_cons, scm_ptr_t car, scm_ptr_t cdr);
            //scm_type_t * scm_car(scm_ptr_t obj);
            DECL_WITH_WRAPPER(scm_car, scm_ptr_t obj);
            //scm_type_t * scm_cdr(scm_ptr_t obj);
            DECL_WITH_WRAPPER(scm_cdr, scm_ptr_t obj);
            //scm_type_t * scm_is_null(scm_ptr_t obj);
            DECL_WITH_WRAPPER(scm_is_null, scm_ptr_t obj);
            //scm_type_t * scm_vector_length(scm_ptr_t obj);
            DECL_WITH_WRAPPER(scm_vector_length, scm_ptr_t obj);
            //scm_type_t * scm_vector_ref(scm_ptr_t obj, scm_ptr_t idx);
            DECL_WITH_WRAPPER(scm_vector_ref, scm_ptr_t obj, scm_ptr_t idx);
            //scm_type_t * scm_plus(scm_type_t * arg0, ...);
            DECL_WITH_WRAPPER(scm_plus, scm_type_t * arg0, ...);
            //scm_type_t * scm_minus(scm_type_t * arg0, ...);
            DECL_WITH_WRAPPER(scm_minus, scm_type_t * arg0, ...);
            //scm_type_t * scm_times(scm_type_t * arg0, ...);
            DECL_WITH_WRAPPER(scm_times, scm_type_t * arg0, ...);
            //scm_type_t * scm_div(scm_type_t * arg0, ...);
            DECL_WITH_WRAPPER(scm_div, scm_type_t * arg0, ...);

            // Note: length does not have to be a native function
            // we can define it in scheme like this:
            // (define (length a) (if (null? a) 0 (+ 1 (length (cdr a)))))
            DECL_WITH_WRAPPER(scm_length, scm_ptr_t list);

            DECL_WITH_WRAPPER(scm_apply, scm_ptr_t func, scm_ptr_t list);

            DECL_WITH_WRAPPER(scm_make_base_nspace);

            DECL_WITH_WRAPPER(scm_eval, scm_ptr_t expr);
        }
    }
}

#endif //LLSCHEME_RUNTIME_H
