#ifndef LLSCHEME_RUNTIME_H
#define LLSCHEME_RUNTIME_H

#define SCM_NULL &(Constant::scm_null)
#define SCM_TRUE &(Constant::scm_true)
#define SCM_FALSE &(Constant::scm_false)

namespace llscm {
    namespace runtime {
#include "runtime/types.hpp"

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

        typedef scm_type_t * (*scm_fnptr_t)(int32_t argc, ...);

        struct scm_func_t {
            int32_t tag;
            int32_t argc;
            scm_fnptr_t fnptr;
        };

        struct scm_vec_t {
            int32_t tag;
            int32_t size;
            scm_type_t * elems[1];
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

        extern "C" {
            extern int32_t exit_code;
            extern scm_type_t * scm_argv;

            scm_type_t * scm_get_arg_vector(int argc, char * argv[]);
            scm_type_t * scm_cmd_args();
            scm_type_t * scm_display(scm_ptr_t obj);
            scm_type_t * scm_plus(int32_t argc, ...);
            scm_type_t * scm_minus(int32_t argc, ...);
            scm_type_t * scm_times(int32_t argc, ...);
            scm_type_t * scm_div(int32_t argc, ...);
            scm_type_t * scm_gt(scm_ptr_t a, scm_ptr_t b);
            scm_type_t * scm_num_eq(scm_ptr_t a, scm_ptr_t b);
            scm_type_t * scm_cons(scm_ptr_t car, scm_ptr_t cdr);
            scm_type_t * scm_car(scm_ptr_t obj);
            scm_type_t * scm_cdr(scm_ptr_t obj);
            scm_type_t * scm_is_null(scm_ptr_t obj);
            scm_type_t * scm_vector_length(scm_ptr_t obj);
            scm_type_t * scm_vector_ref(scm_ptr_t obj, scm_ptr_t idx);
        }
    }
}

#endif //LLSCHEME_RUNTIME_H
