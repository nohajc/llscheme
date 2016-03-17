#include <cstdint>
#include "../include/runtime.h"

namespace llscm {
    namespace runtime {
        const char * Symbols::malloc = "scm_alloc";
        const char * Symbols::cons = "scm_cons";
        const char * Symbols::car = "scm_car";
        const char * Symbols::cdr = "scm_cdr";
        const char * Symbols::isNull = "scm_is_null";
        const char * Symbols::plus = "scm_plus";
        const char * Symbols::minus = "scm_minus";
        const char * Symbols::times = "scm_times";
        const char * Symbols::div = "scm_div";
        const char * Symbols::gt = "scm_gt";
        const char * Symbols::print = "scm_print";
    }
}
