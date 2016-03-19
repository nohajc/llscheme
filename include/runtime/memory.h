#ifndef LLSCHEME_MEMORY_HPP
#define LLSCHEME_MEMORY_HPP

#include <cstdint>
#include "../runtime.h"


namespace llscm {
    namespace runtime {
        extern "C" {
            scm_type_t * alloc_int(int64_t value);
            scm_type_t * alloc_float(double value);
        }
    }
}

#endif //LLSCHEME_MEMORY_HPP
