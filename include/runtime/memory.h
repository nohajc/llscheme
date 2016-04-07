#ifndef LLSCHEME_MEMORY_HPP
#define LLSCHEME_MEMORY_HPP

#include <cstdint>
#include "../runtime.h"


namespace llscm {
    class ScmEnv;

    namespace runtime {

        // Wrapper class for objects that are
        // to be automatically garbage collected.
        template<class C>
        class GCed: public C, public virtual gc_cleanup {
            static std::set<GCed<C>*> instances;
        public:
            template<typename ...Args>
            GCed(Args && ...args): C(std::forward<Args>(args)...), gc_cleanup() {
                instances.insert(this);
            }
            virtual ~GCed() {
                instances.erase(this);
            }

            // If the GC doesn't delete all of the instances
            // We can still call this manual cleanup at exit
            static void cleanup() {
                D(std::cerr << "Called manual cleanup!" << std::endl);
                for(auto obj: instances) {
                    D(std::cerr << "Deleted instance." << std::endl);
                    delete obj;
                }
            }
        };

        template<class C>
        std::set<GCed<C>*> GCed<C>::instances;

        void mem_cleanup();

        extern "C" {
            scm_type_t * alloc_int(int64_t value);
            scm_type_t * alloc_float(double value);
            scm_type_t * alloc_vec(int32_t size);
            scm_type_t * alloc_str(const char * str);
            scm_type_t * alloc_func(int32_t argc, scm_fnptr_t fnptr,
                                    al_wrapper_t wrfnptr, scm_type_t ** ctxptr);
            scm_type_t * alloc_cons(scm_type_t * car, scm_type_t * cdr);
            scm_type_t * alloc_nspace(ScmEnv * env);
            scm_type_t ** alloc_heap_storage(int32_t size);
        }
    }
}

#endif //LLSCHEME_MEMORY_HPP
