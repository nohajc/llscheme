#ifndef LLSCHEME_ENVIRONMENT_HPP
#define LLSCHEME_ENVIRONMENT_HPP

#include <unordered_map>
#include <string>
#include <memory>
#include "ast.hpp"
#include "parser.hpp"

namespace llscm {
    using namespace std;

    class ScmEnv: public enable_shared_from_this<ScmEnv> {
        bool err_flag;
        shared_ptr<ScmEnv> parent_env;
        ScmEnv * top_level_env;
        unordered_map<ScmSym, P_ScmObj> binding;
        unordered_map<string, uint32_t> uniq_id;
    public:
        ScmProg & prog;
        P_ScmObj context; // Function using this environment or nullptr

        ScmEnv(ScmProg & p, P_ScmEnv penv = nullptr);

        string getUniqID(const string & name);
        // TODO: instead of def_env, we should return information about the number of levels
        // (function environments) visited when searching the symbol binding.
        // In case of closures we need to know whether the binding was found
        // in the parent environment or in the parent of the parent etc.
        // Special value will be returned for global binding (in the top_level_env).
        P_ScmObj get(P_ScmObj k, P_ScmEnv * def_env = nullptr);
        P_ScmObj get(ScmSym * sym, P_ScmEnv * def_env = nullptr);
        bool set(P_ScmObj k, P_ScmObj obj);
        bool set(const string & k, P_ScmObj obj);
        void error(const string & msg);
        bool fail() {
            return err_flag;
        }
    };

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog);
}

#endif //LLSCHEME_ENVIRONMENT_HPP
