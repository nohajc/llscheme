#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <cinttypes>
#include "../../include/runtime.h"
#include "../../include/runtime/memory.h"
#include "../../include/runtime/error.h"

#define EPSILON 10E-9

namespace llscm {
    namespace runtime {
        scm_type_t Constant::scm_null = { S_NIL };
        scm_type_t Constant::scm_true = { S_TRUE };
        scm_type_t Constant::scm_false = { S_FALSE };

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

        scm_type_t * scm_plus(scm_type_t * arg0, ...) {
            va_list ap;
            int64_t sum = 0;
            double fsum = 0;
            bool is_int = true;
            scm_ptr_t obj = arg0;

            va_start(ap, arg0);
            while (obj.asType != nullptr) {
                if (is_int) {
                    if (obj->tag == S_INT) {
                        sum += obj.asInt->value;
                    }
                    else if (obj->tag == S_FLOAT) {
                        fsum = sum + obj.asFloat->value;
                        is_int = false;
                    }
                    else {
                        INVALID_ARG_TYPE();
                    }
                }
                else {
                    if (obj->tag == S_INT) {
                        fsum += obj.asInt->value;
                    }
                    else if (obj->tag == S_FLOAT) {
                        fsum += obj.asFloat->value;
                    }
                    else {
                        INVALID_ARG_TYPE();
                    }
                }
                obj = va_arg(ap, scm_type_t*);
            }
            va_end(ap);

            // TODO: implement exceptions

            if (is_int) {
                return alloc_int(sum);
            }
            return alloc_float(fsum);
        }

        scm_type_t * scm_minus(scm_type_t * arg0, ...) {
            va_list ap;
            int64_t diff = 0;
            double fdiff = 0;
            bool is_int = true;
            scm_ptr_t obj = arg0;

            if (arg0 == nullptr) { // Zero arguments
                // TODO: is it safe to return from the function before
                // consuming all arguments and calling va_end?
                WRONG_ARG_NUM();
            }

            va_start(ap, arg0);

            //obj = va_arg(ap, scm_type_t*);
            if (obj->tag == S_INT) {
                diff = obj.asInt->value;
            }
            else if (obj->tag == S_FLOAT) {
                fdiff = obj.asFloat->value;
                is_int = false;
            }
            else {
                INVALID_ARG_TYPE();
            }

            obj = va_arg(ap, scm_type_t*);
            if (obj.asType == nullptr) { // One argument
                if (is_int) {
                    return alloc_int(-diff);
                }
                return alloc_float(-fdiff);
            }

            while (obj.asType != nullptr) {
                if (is_int) {
                    if (obj->tag == S_INT) {
                        diff -= obj.asInt->value;
                    }
                    else if (obj->tag == S_FLOAT) {
                        fdiff = diff - obj.asFloat->value;
                        is_int = false;
                    }
                    else {
                        INVALID_ARG_TYPE();
                    }
                }
                else {
                    if (obj->tag == S_INT) {
                        fdiff -= obj.asInt->value;
                    }
                    else if (obj->tag == S_FLOAT) {
                        fdiff -= obj.asFloat->value;
                    }
                    else {
                        INVALID_ARG_TYPE();
                    }
                }
                obj = va_arg(ap, scm_type_t*);
            }

            va_end(ap);
            if (is_int) {
                return alloc_int(diff);
            }
            return alloc_float(fdiff);
        }

        scm_type_t * scm_times(scm_type_t * arg0, ...) {
            return nullptr;
        }

        scm_type_t * scm_div(scm_type_t * arg0, ...) {
            return nullptr;
        }

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