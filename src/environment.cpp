#include <llvm/ADT/STLExtras.h>
#include "../include/environment.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog) {
        shared_ptr<ScmEnv> env = make_shared<ScmEnv>(prog);

        env->set(ScmSym("cons"), make_shared<ScmConsFunc>());
        env->set(ScmSym("car"), make_shared<ScmCarFunc>());
        env->set(ScmSym("cdr"), make_shared<ScmCdrFunc>());
        env->set(ScmSym("+"), make_shared<ScmPlusFunc>());
        env->set(ScmSym("-"), make_shared<ScmMinusFunc>());

        return env;
    }

    P_ScmObj ScmEnv::get(const ScmSym & k) {
        auto elem_it = binding.find(k);
        if (elem_it == binding.end()) {
            if (parent_env) {
                return parent_env->get(k);
            }
            return nullptr;
        }
        return elem_it->second;
    }

    void ScmEnv::set(const ScmSym & k, P_ScmObj obj) {
        binding[k] = obj;
    }

    void ScmEnv::error(const string &msg) {
        cout << "Error: " << msg << endl;
        err_flag = true;
    }

}
