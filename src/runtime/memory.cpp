#include <gc.h>
#include <cstring>
#include "../../include/runtime/memory.h"

namespace llscm {
    namespace runtime {
        scm_type_t * alloc_int(int64_t value) {
            scm_ptr_t obj = GC_MALLOC(sizeof(scm_int_t));
            obj->tag = S_INT;
            obj.asInt->value = value;
            return obj;
        }

        scm_type_t * alloc_float(double value) {
            scm_ptr_t obj = GC_MALLOC(sizeof(scm_float_t));
            obj->tag = S_FLOAT;
            obj.asFloat->value = value;
            return obj;
        }

        scm_type_t * alloc_vec(int32_t size) {
            uint32_t vec_alloc_size = sizeof(scm_vec_t);
            if (size > 1) {
                vec_alloc_size += (size - 1) * sizeof(scm_type_t*);
            }

            scm_ptr_t obj = GC_MALLOC(vec_alloc_size);
            obj->tag = S_VEC;
            obj.asVec->size = size;

            for (int i = 0; i < size; i++) {
                obj.asVec->elems[i] = SCM_NULL;
            }

            return obj;
        }

        scm_type_t * alloc_str(const char *str) {
            size_t len = strlen(str);
            uint32_t str_alloc_size = sizeof(scm_str_t);
            str_alloc_size += len * sizeof(char);

            scm_ptr_t obj = GC_MALLOC(str_alloc_size);
            obj->tag = S_STR;
            obj.asStr->len = (int32_t)len;
            strcpy(obj.asStr->str, str);

            return obj;
        }

        scm_type_t ** alloc_heap_storage(int32_t size) {
            return (scm_type_t**)GC_MALLOC(size * sizeof(scm_type_t*));
        }

        scm_type_t * alloc_func(int32_t argc, scm_fnptr_t fnptr,
                                al_wrapper_t wrfnptr, scm_type_t ** ctxptr) {
            scm_ptr_t obj = GC_MALLOC(sizeof(scm_func_t));
            obj->tag = S_FUNC;
            obj.asFunc->argc = argc;
            obj.asFunc->fnptr = fnptr;
            obj.asFunc->wrfnptr = wrfnptr;
            obj.asFunc->ctxptr = ctxptr;

            return obj;
        }

        scm_type_t * alloc_cons(scm_type_t * car, scm_type_t * cdr) {
            scm_ptr_t obj = GC_MALLOC(sizeof(scm_cons_t));
            obj->tag = S_CONS;
            obj.asCons->car = car;
            obj.asCons->cdr = cdr;

            return obj;
        }


    }
}
