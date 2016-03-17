#ifndef LLSCHEME_RUNTIME_H
#define LLSCHEME_RUNTIME_H

namespace llscm {
    namespace runtime {
        struct Symbols {
            static const char *malloc;
            static const char *cons;
            static const char *car;
            static const char *cdr;
            static const char *isNull;
            static const char *plus;
            static const char *minus;
            static const char *times;
            static const char *div;
            static const char *gt;
            static const char *print;
        };

        struct scm_type_t {
            int32_t tag;
        };

        struct scm_int_t {
            int32_t tag;
            int64_t value;
        };

        struct scm_float_t {
            int32_t tag;
            double value;
        };

        struct scm_str_t {
            int32_t tag;
            int32_t len;
            char str[1];
        };

        struct scm_sym_t {
            int32_t tag;
            int32_t len;
            char sym[1];
        };

        struct scm_cons_t {
            int32_t tag;
            scm_type_t * car;
            scm_type_t * cdr;
        };

        typedef scm_type_t * (*scm_fnptr_t)(int32_t argc, ...);

        struct scm_func_t {
            int32_t tag;
            int32_t argc;
            scm_fnptr_t fnptr;
        };
    }
}

#endif //LLSCHEME_RUNTIME_H
