#include "../../include/runtime/error.h"
#include "../../include/runtime.h"

#include <cstdlib>
#include <cstdio>

namespace llscm {
    namespace runtime {
        void error_not_a_function(scm_type_t * obj) {
            RUNTIME_ERROR("Cannot call object of type %s, expected %s.\n", TagName[obj->tag], TagName[S_FUNC]);
        }

        void error_wrong_arg_num(scm_func_t * func, int32_t argc) {
            RUNTIME_ERROR("Called function expects %d arguments, %d given.\n", func->argc, argc);
        }

    }
}

