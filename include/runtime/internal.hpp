#ifndef LLSCHEME_INTERNAL_H_HPP
#define LLSCHEME_INTERNAL_H_HPP

#include <cstdio>
#include <cstdlib>
#include "../runtime.h"
#include "../runtime/memory.h"
#include "../runtime/error.h"

namespace llscm {
    namespace runtime {
        template<typename F1, typename F2>
        inline scm_type_t * internal_scm_plus(F1 first_arg, F2 next_arg) {
            int64_t sum = 0;
            double fsum = 0;
            bool is_int = true;
            scm_ptr_t obj = first_arg();

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
                obj = next_arg();
            }

            if (is_int) {
                return alloc_int(sum);
            }
            return alloc_float(fsum);
        }

        template<typename F1, typename F2>
        inline scm_type_t * internal_scm_minus(F1 first_arg, F2 next_arg) {
            int64_t diff = 0;
            double fdiff = 0;
            bool is_int = true;
            scm_type_t * arg0 = first_arg();
            scm_ptr_t obj = arg0;

            if (arg0 == nullptr) { // Zero arguments
                // TODO: is it safe to return from the function before
                // consuming all arguments and calling va_end?
                WRONG_ARG_NUM();
            }

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

            obj = next_arg();
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
                obj = next_arg();
            }

            if (is_int) {
                return alloc_int(diff);
            }
            return alloc_float(fdiff);
        }

        template<typename F1, typename F2>
        inline scm_type_t * internal_scm_times(F1 first_arg, F2 next_arg) {
            return SCM_NULL;
        };

        template<typename F1, typename F2>
        inline scm_type_t * internal_scm_div(F1 first_arg, F2 next_arg) {
            return SCM_NULL;
        };
    }
}

#endif //LLSCHEME_INTERNAL_H_HPP
