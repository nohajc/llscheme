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
        ScmProg & prog;

    public:
        ScmEnv(ScmProg & p, shared_ptr<ScmEnv> penv = nullptr):
                prog(p), parent_env(penv) {}

        P_ScmObj get(const ScmSym & k);
        void set(const ScmSym & k, P_ScmObj obj);
        void error(const string & msg);
    };

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog);
}

#endif //LLSCHEME_ENVIRONMENT_HPP
