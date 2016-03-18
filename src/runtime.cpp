#include <cstdio>
#include <cstdint>
#include "../include/runtime.h"

#define SCM_NULL &(Constant::scm_null)

namespace llscm {
    namespace runtime {
        const char * Symbol::malloc = "scm_alloc";
        const char * Symbol::cons = "scm_cons";
        const char * Symbol::car = "scm_car";
        const char * Symbol::cdr = "scm_cdr";
        const char * Symbol::isNull = "scm_is_null";
        const char * Symbol::plus = "scm_plus";
        const char * Symbol::minus = "scm_minus";
        const char * Symbol::times = "scm_times";
        const char * Symbol::div = "scm_div";
        const char * Symbol::gt = "scm_gt";
        const char * Symbol::print = "scm_print";

        scm_type_t Constant::scm_null = { NIL };

        scm_type_t * scm_print(scm_type_t * str) {
            if (str->tag == STR) {
                puts(((scm_str_t*)str)->str);
            }

            return SCM_NULL;
        }
    }
}

#undef SCM_NULL