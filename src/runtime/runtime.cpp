#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cinttypes>
#include "../../include/runtime.h"
#include "../../include/runtime/internal.hpp"

#define EPSILON 10E-9

namespace llscm {
    namespace runtime {
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

#define SCM_ARGLIST_WRAPPER(func) \
        scm_type_t * argl_##func(scm_type_t ** arg_list) { \
            int32_t idx = 1; \
            return internal_##func( \
                [arg_list] () { return arg_list[0]; }, \
                [arg_list, &idx] () { return arg_list[idx++]; } \
            ); \
        }

        SCM_VARARGS_WRAPPER(scm_plus);
        SCM_ARGLIST_WRAPPER(scm_plus);

        SCM_VARARGS_WRAPPER(scm_minus);
        SCM_ARGLIST_WRAPPER(scm_minus);

        SCM_VARARGS_WRAPPER(scm_times);
        SCM_ARGLIST_WRAPPER(scm_times);

        SCM_VARARGS_WRAPPER(scm_div);
        SCM_ARGLIST_WRAPPER(scm_div);

        /*scm_type_t * scm_times(scm_type_t * arg0, ...) {
            return nullptr;
        }

        scm_type_t * scm_div(scm_type_t * arg0, ...) {
            return nullptr;
        }*/

        scm_type_t * scm_display(scm_ptr_t obj) {
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
                default:
                    INVALID_ARG_TYPE();
            }
            // TODO: rest of the types including CONS

            return SCM_NULL;
        }

        /*scm_type_t * scm_minus(scm_type_t * arg0, ...) {
            va_list ap;
            va_start(ap, arg0);

            scm_type_t * res = internal_scm_minus(
                    [&arg0] () { return arg0; },
                    [&ap] () { return va_arg(ap, scm_type_t*); }
            );

            va_end(ap);
            return res;
        }*/

        scm_type_t * scm_gt(scm_ptr_t a, scm_ptr_t b) {
            return nullptr;
        }

        scm_type_t * scm_num_eq(scm_ptr_t a, scm_ptr_t b) {
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

        scm_type_t * scm_cons(scm_ptr_t car, scm_ptr_t cdr) {
            return nullptr;
        }

        scm_type_t * scm_car(scm_ptr_t obj) {
            return nullptr;
        }

        scm_type_t * scm_cdr(scm_ptr_t obj) {
            return nullptr;
        }

        scm_type_t * scm_is_null(scm_ptr_t obj) {
            return nullptr;
        }

        scm_type_t * scm_get_arg_vector(int argc, char * argv[]) {
            scm_ptr_t obj = alloc_vec(argc - 1);
            for (int i = 1; i < argc; i++) {
                obj.asVec->elems[i - 1] = alloc_str(argv[i]);
            }
            return obj;
        }

        scm_type_t * scm_vector_length(scm_ptr_t obj) {
            if (obj->tag != S_VEC) {
                INVALID_ARG_TYPE();
            }

            scm_ptr_t len = alloc_int(obj.asVec->size);
            return len;
        }

        scm_type_t * scm_vector_ref(scm_ptr_t obj, scm_ptr_t idx) {
            if (obj->tag != S_VEC) {
                INVALID_ARG_TYPE();
            }

            if (idx->tag != S_INT) {
                INVALID_ARG_TYPE();
            }

            return obj.asVec->elems[idx.asInt->value];
        }

        scm_type_t * scm_cmd_args() {
            return scm_argv;
        }

    }
}

#undef RUNTIME_ERROR