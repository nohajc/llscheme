#ifndef LLSCHEME_ENVIRONMENT_HPP
#define LLSCHEME_ENVIRONMENT_HPP

#include <unordered_map>
#include <memory>
#include "ast.hpp"
#include "parser.hpp"

namespace llscm {
    using namespace std;

    class ScmEnv {
        bool err_flag;
        shared_ptr<ScmEnv> parent_env;
        unordered_map<ScmSym, P_ScmObj> binding;
    public:
        ScmProg & prog;
        P_ScmObj context;

        ScmEnv(ScmProg & p, P_ScmEnv penv = nullptr):
                prog(p), parent_env(penv) {}

        P_ScmObj get(P_ScmObj k, P_ScmEnv * def_env = nullptr);
        P_ScmObj get(ScmSym * sym, P_ScmEnv * def_env = nullptr);
        bool set(P_ScmObj k, P_ScmObj obj);
        bool set(const string & k, P_ScmObj obj);
        void error(const string & msg);
    };

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog);
}

#endif //LLSCHEME_ENVIRONMENT_HPP
