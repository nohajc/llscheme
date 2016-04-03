#ifndef LLSCHEME_MEMORY_HPP
#define LLSCHEME_MEMORY_HPP

#include <cstdint>
#include "../runtime.h"


namespace llscm {
    namespace runtime {
        extern "C" {
            scm_type_t * alloc_int(int64_t value);
            scm_type_t * alloc_float(double value);
            scm_type_t * alloc_vec(int32_t size);
            scm_type_t * alloc_str(const char * str);
            scm_type_t * alloc_func(int32_t argc, scm_fnptr_t fnptr,
                                    al_wrapper_t wrfnptr, scm_type_t ** ctxptr);
            scm_type_t * alloc_cons(scm_type_t * car, scm_type_t * cdr);
            scm_type_t ** alloc_heap_storage(int32_t size);
        }
    }
}

#endif //LLSCHEME_MEMORY_HPP
