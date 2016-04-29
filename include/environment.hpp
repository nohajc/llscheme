#ifndef LLSCHEME_ENVIRONMENT_HPP
#define LLSCHEME_ENVIRONMENT_HPP

#include <unordered_map>
#include <string>
#include <memory>
#include "ast.hpp"
#include "parser.hpp"
#include "libmetainfo.hpp"

namespace llscm {
    using namespace std;

    typedef shared_ptr<pair<int, ScmFunc*>> ScmLoc;

    class ScmNameGen {
        unordered_map<string, uint32_t> uniq_id;
    public:
        string getUniqID(const string & name);
    };

    class ScmEnv: public enable_shared_from_this<ScmEnv> {
        bool err_flag;
        ScmEnv * top_level_env;
        unordered_map<ScmSym, P_ScmObj> binding;
        ScmNameGen namegen;
    public:
        static int GlobalLevel;
        ScmProg * prog;
        list<P_ScmObj>::iterator prog_begin;
        shared_ptr<ScmEnv> parent_env;
        P_ScmObj context; // Function using this environment or nullptr
        bool link_lib;

        ScmEnv(ScmProg * p, P_ScmEnv penv = nullptr);

        void setProg(ScmProg & p);

        string getUniqID(const string & name);
        // We  return information about the number of levels
        // (function environments) visited when searching the symbol binding.
        // In case of closures we need to know whether the binding was found
        // in the parent environment or in the parent of the parent etc.
        // Special value will be returned for global binding (in the top_level_env).
        // In addition to number of levels, we also have to return the pointer
        // stored in context, so that the caller can inform ScmFunc of a new heap local
        // (in case of reference to captured object).
        P_ScmObj get(P_ScmObj k, ScmLoc * loc = nullptr);
        P_ScmObj get(ScmSym * sym, ScmLoc * loc = nullptr);
        ScmFunc * defInFunc();
        bool set(P_ScmObj k, P_ScmObj obj);
        bool set(const string & k, P_ScmObj obj);
        void error(const string & msg);
        bool fail() {
            return err_flag;
        }
        bool isGlobal() {
            return this == top_level_env;
        }
        // This method has to be run on the environment in case
        // we want to use it again to compile different program.
        // Otherwise it would still contain references to the
        // previously generated LLVM Module.
        void setGlobalsAsExternal();

        struct CapturedRef {
            P_ScmEnv env;
            shared_ptr<ScmRef> ref;
        };
        unordered_multimap<ScmSym, CapturedRef> & getUnRefs() {
            return top_level_env->urefs;
        };

        struct CapturedObj {
            P_ScmEnv env;
            P_ScmObj obj;
        };
        unordered_multimap<ScmRef*, CapturedObj> & evalAgain() {
            return top_level_env->eval_again;
        };

        void checkUnRefs();
    private:
        // Unresolved references
        // (there can be multiple references to the same symbol, of course)
        unordered_multimap<ScmSym, CapturedRef> urefs;
        unordered_multimap<ScmRef*, CapturedObj> eval_again;
    };

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog);
    void initGlobalEnvironment(ScmEnv * env, void * lib_blob = nullptr);
}

#endif //LLSCHEME_ENVIRONMENT_HPP
