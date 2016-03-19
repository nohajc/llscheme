#include <gc.h>
#include "../../include/runtime/memory.h"

namespace llscm {
    namespace runtime {
        scm_type_t * alloc_int(int64_t value) {
            scm_ptr_t obj = GC_MALLOC(sizeof(scm_int_t));
            obj->tag = INT;
            obj.asInt->value = value;
            return obj;
        }

        scm_type_t * alloc_float(double value) {
            scm_ptr_t obj = GC_MALLOC(sizeof(scm_float_t));
            obj->tag = FLOAT;
            obj.asFloat->value = value;
            return obj;
        }
    }
}
