#ifndef LLSCHEME_ERROR_H
#define LLSCHEME_ERROR_H

#include <cstdint>
#include "../runtime.h"

#define RUNTIME_ERROR(fmt, ...) do { \
    fprintf(stderr, "Error: " fmt, __VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while(0)

// Error messages for native functions

#define INVALID_ARG_TYPE() RUNTIME_ERROR("Invalid type of argument given to %s.\n", __func__)
#define WRONG_ARG_NUM() RUNTIME_ERROR("Wrong number of arguments given to %s.\n", __func__)
#define EVAL_FAILED() RUNTIME_ERROR("%s: Evaluation of quoted expression failed.", __func__)

namespace llscm {
    namespace runtime {
        extern "C" {
            // Error messages for llscheme functions
            void error_not_a_function(scm_type_t * obj);
            void error_wrong_arg_num(scm_func_t * func, int32_t argc);
        }
    }
}

#endif //LLSCHEME_ERROR_H
