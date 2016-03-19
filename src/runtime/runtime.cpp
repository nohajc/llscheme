#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include "../../include/runtime.h"
#include "../../include/runtime/memory.h"

#define SCM_NULL &(Constant::scm_null)

namespace llscm {
    namespace runtime {
        scm_type_t Constant::scm_null = { NIL };

        scm_type_t * scm_print(scm_ptr_t obj) {
            if (obj->tag == STR) {
                printf("%s", obj.asStr->str);
            }
            else if(obj->tag == SYM) {
                printf("%s", obj.asSym->sym);
            }
            else if(obj->tag == INT) {
                printf("%li", obj.asInt->value);
            }
            else if(obj->tag == FLOAT) {
                printf("%g", obj.asFloat->value);
            }

            return SCM_NULL;
        }

        scm_type_t * scm_plus(int32_t argc, ...) {
            va_list ap;
            int32_t i;
            int64_t sum = 0;
            double fsum = 0;
            bool is_int = true;
            bool type_error = false;

            scm_ptr_t obj;

            va_start(ap, argc);
            for (i = 0; i < argc; ++i) {
                obj = va_arg(ap, scm_type_t*);
                if (is_int) {
                    if (obj->tag == INT) {
                        sum += obj.asInt->value;
                    }
                    else if (obj->tag == FLOAT) {
                        fsum = sum + obj.asFloat->value;
                        is_int = false;
                    }
                    else {
                        type_error = true;
                    }
                }
                else {
                    if (obj->tag == INT) {
                        fsum += obj.asInt->value;
                    }
                    else if (obj->tag == FLOAT) {
                        fsum += obj.asFloat->value;
                    }
                    else {
                        type_error = true;
                    }
                }
            }
            va_end(ap);

            // TODO: implement exceptions or a special error type to return
            if (type_error) {
                return SCM_NULL;
            }
            if (is_int) {
                return alloc_int(sum);
            }
            return alloc_float(fsum);
        }


    }
}

#undef SCM_NULL