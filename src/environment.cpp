#include <sstream>
#include <llvm/ADT/STLExtras.h>
#include "../include/environment.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog) {
        shared_ptr<ScmEnv> env = make_shared<ScmEnv>(prog);

        env->set("cons", make_shared<ScmConsFunc>());
        env->set("car", make_shared<ScmCarFunc>());
        env->set("cdr", make_shared<ScmCdrFunc>());
        env->set("+", make_shared<ScmPlusFunc>());
        env->set("-", make_shared<ScmMinusFunc>());
        env->set("null?", make_shared<ScmNullFunc>());
        env->set(">", make_shared<ScmGtFunc>());
        env->set("*", make_shared<ScmTimesFunc>());
        env->set("/", make_shared<ScmDivFunc>());
        env->set("print", make_shared<ScmPrintFunc>());

        return env;
    }

    ScmEnv::ScmEnv(ScmProg & p, P_ScmEnv penv): prog(p), parent_env(penv) {
        if (!penv) {
            top_level_env = this;
        }
        else {
            top_level_env = penv->top_level_env;
        }
    }

    P_ScmObj ScmEnv::get(P_ScmObj k, P_ScmEnv * def_env) {
        ScmSym * sym = dynamic_cast<ScmSym*>(k.get());
        return get(sym, def_env);
    }

    P_ScmObj ScmEnv::get(ScmSym * sym, P_ScmEnv * def_env) {
        auto elem_it = binding.find(*sym);
        if (elem_it == binding.end()) {
            if (parent_env) {
                return parent_env->get(sym);
            }
            return nullptr;
        }
        // If valid pointer given in def_env, return
        // the environment where we've found the symbol binding.
        if (def_env) *def_env = shared_from_this();
        return elem_it->second;
    }

    bool ScmEnv::set(P_ScmObj k, P_ScmObj obj) {
        ScmSym * sym = dynamic_cast<ScmSym*>(k.get());
        if (!sym) {
            return false;
        }
        binding[*sym] = obj;
        return true;
    }

    bool ScmEnv::set(const string & k, P_ScmObj obj) {
        unique_ptr<ScmSym> sym = make_unique<ScmSym>(k);
        if (!sym) {
            return false;
        }
        binding[*sym] = obj;
        return true;
    }

    void ScmEnv::error(const string &msg) {
        cout << "Error: " << msg << endl;
        top_level_env->err_flag = err_flag = true;
    }

    string ScmEnv::getUniqID(const string & name) {
        auto kv = top_level_env->uniq_id.find(name);
        stringstream ss;
        if (kv != top_level_env->uniq_id.end()) {
            kv->second++;
            ss << name << kv->second;
        }
        else {
            uniq_id[name] = 0;
            ss << name << 0;
        }
        return ss.str();
    }
}