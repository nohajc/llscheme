#include <sstream>
#include <llvm/ADT/STLExtras.h>
#include "../include/environment.hpp"
#include "../include/debug.hpp"

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
        env->set("=", make_shared<ScmNumEqFunc>());
        env->set("*", make_shared<ScmTimesFunc>());
        env->set("/", make_shared<ScmDivFunc>());
        env->set("display", make_shared<ScmDisplayFunc>());
        env->set("current-command-line-arguments", make_shared<ScmCmdArgsFunc>());
        env->set("vector-length", make_shared<ScmVecLenFunc>());
        env->set("vector-ref", make_shared<ScmVecRefFunc>());
        env->set("apply", make_shared<ScmApplyFunc>());
        env->set("length", make_shared<ScmLengthFunc>());

        return env;
    }

    int ScmEnv::GlobalLevel = -2;

    ScmEnv::ScmEnv(ScmProg & p, P_ScmEnv penv): prog(p), parent_env(penv) {
        if (!penv) {
            top_level_env = this;
        }
        else {
            top_level_env = penv->top_level_env;
        }
        err_flag = false;
        prog_begin = prog.begin();
        context = nullptr;
    }

    P_ScmObj ScmEnv::get(P_ScmObj k, ScmLoc * loc) {
        ScmSym * sym = dynamic_cast<ScmSym*>(k.get());
        return get(sym, loc);
    }

    P_ScmObj ScmEnv::get(ScmSym * sym, ScmLoc * loc) {
        bool find_func = false;
        ScmFunc * func = nullptr;
        if (loc) {
            find_func = true;
            if (context && context->t == T_FUNC) {
                func = DPC<ScmFunc>(context).get();
            }

            if (!*loc) {
                *loc = make_shared<pair<int, ScmFunc*>>(-1, func);
            }

            if (func) {
                (*loc)->first += 1;
                (*loc)->second = func;
            }

            if (this == top_level_env) {
                (*loc)->first = GlobalLevel;
                find_func = false;
            }
        }

        auto elem_it = binding.find(*sym);
        if (elem_it == binding.end()) {
            if (parent_env) {
                return parent_env->get(sym, loc);
            }
            return nullptr;
        }

        if (!func && find_func && *loc) {
            // We're inside a let block
            D(cerr << "FIND_FUNC" << endl);
            ScmEnv * p_env = this;
            while (p_env) {
                if (p_env == top_level_env) {
                    (*loc)->first = GlobalLevel;
                    break;
                }
                if (p_env->context && p_env->context->t == T_FUNC) {
                    (*loc)->first += 1;
                    (*loc)->second = DPC<ScmFunc>(p_env->context).get();;
                    break;
                }
                p_env = p_env->parent_env.get();
            }
        }
        // If valid pointer given in def_env, return
        // the environment where we've found the symbol binding.
        //if (def_env) *def_env = shared_from_this();
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
            top_level_env->uniq_id[name] = 0;
            ss << name << 0;
        }
        return ss.str();
    }

    ScmFunc * ScmEnv::defInFunc() {
        ScmFunc * func = nullptr;
        ScmEnv * p_env = this;
        while (p_env) {
            if (p_env->context && p_env->context->t == T_FUNC) {
                func = DPC<ScmFunc>(p_env->context).get();;
                break;
            }
            p_env = p_env->parent_env.get();
        }
        return func;
    }


}
