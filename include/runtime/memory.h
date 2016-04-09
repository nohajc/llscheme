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
            static std::shared_ptr<bool> anchor;
        public:
            template<typename ...Args>
            GCed(Args && ...args): C(std::forward<Args>(args)...), gc_cleanup() {
                instances.insert(this);
            }
            virtual ~GCed() {
                instances.erase(this);
            }

            std::shared_ptr<C> getSharedPtr() {
                // We use the shared_ptr aliasing constructor
                // to get something like "shared_from_this()"
                // for an object managed by the Boehm's GC
                return std::shared_ptr<C>(anchor, this);
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

        template<class C>
        std::shared_ptr<bool> GCed<C>::anchor = std::make_shared<bool>(true);

        void mem_cleanup();

        extern "C" {
            scm_type_t * alloc_int(int64_t value);
            scm_type_t * alloc_float(double value);
            scm_type_t * alloc_vec(int32_t size);
            scm_type_t * alloc_str(const char * str);
            scm_type_t * alloc_func(int32_t argc, scm_fnptr_t fnptr,
                                    al_wrapper_t wrfnptr, scm_type_t ** ctxptr);
            scm_type_t * alloc_cons(scm_type_t * car, scm_type_t * cdr);
            scm_type_t * alloc_nspace(GCed<ScmEnv> * env);
            scm_type_t ** alloc_heap_storage(int32_t size);
        }
    }
}

#endif //LLSCHEME_MEMORY_HPP
