#ifndef LLSCHEME_INTERNAL_H_HPP
#define LLSCHEME_INTERNAL_H_HPP

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cassert>
#include "../runtime.h"
#include "../runtime/memory.h"
#include "../runtime/error.h"
#include "../runtime/meta.hpp"

namespace llscm {
    namespace runtime {
        template<typename>
        struct Arguments;

        template<int... idx>
        struct Arguments<RangeElems<idx...>> {
            template<typename T, T * func>
            inline static scm_type_t * argl_wrapper(scm_type_t ** arg_list) {
                // Number of arguments will be checked by the caller who
                // has the complete scm_func object.
                return func(arg_list[idx]...);
            };
        };

        template<int E>
        struct Arguments<Range<E>>: Arguments<typename Range<E>::type> {};

        template<int C>
        struct Argc: Arguments<Range<C>> {};

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
            int64_t prod = 1;
            double fprod = 1.0;
            bool is_int = true;
            scm_ptr_t obj = first_arg();

            while (obj.asType != nullptr) {
                if (is_int) {
                    if (obj->tag == S_INT) {
                        prod *= obj.asInt->value;
                    }
                    else if (obj->tag == S_FLOAT) {
                        fprod = prod * obj.asFloat->value;
                        is_int = false;
                    }
                    else {
                        INVALID_ARG_TYPE();
                    }
                }
                else {
                    if (obj->tag == S_INT) {
                        fprod *= obj.asInt->value;
                    }
                    else if (obj->tag == S_FLOAT) {
                        fprod *= obj.asFloat->value;
                    }
                    else {
                        INVALID_ARG_TYPE();
                    }
                }
                obj = next_arg();
            }

            if (is_int) {
                return alloc_int(prod);
            }
            return alloc_float(fprod);
        }

        template<typename F1, typename F2>
        inline scm_type_t * internal_scm_div(F1 first_arg, F2 next_arg) {
            double fquot;
            scm_type_t * arg0 = first_arg();
            scm_ptr_t obj = arg0;

            if (arg0 == nullptr) { // Zero arguments
                WRONG_ARG_NUM();
            }

            //obj = va_arg(ap, scm_type_t*);
            if (obj->tag == S_INT) {
                fquot = obj.asInt->value;
            }
            else if (obj->tag == S_FLOAT) {
                fquot = obj.asFloat->value;
            }
            else {
                INVALID_ARG_TYPE();
            }

            obj = next_arg();
            if (obj.asType == nullptr) { // One argument
                return alloc_float(1 / fquot);
            }

            while (obj.asType != nullptr) {
                if (obj->tag == S_INT) {
                    fquot /= obj.asInt->value;
                }
                else if (obj->tag == S_FLOAT) {
                    fquot /= obj.asFloat->value;
                }
                else {
                    INVALID_ARG_TYPE();
                }
                obj = next_arg();
            }

            return alloc_float(fquot);
        }

        template<typename F>
        void list_foreach(scm_ptr_t list, F func) {
            scm_ptr_t cell = list;
            while(cell->tag != S_NIL) {
                assert(cell->tag == S_CONS);

                func(cell);
                cell = cell.asCons->cdr;
            }
        }
    }
}

#endif //LLSCHEME_INTERNAL_H_HPP
