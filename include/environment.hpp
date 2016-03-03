#ifndef LLSCHEME_ENVIRONMENT_HPP
#define LLSCHEME_ENVIRONMENT_HPP

#include <unordered_map>
#include <memory>
#include "ast.hpp"

namespace llscm {
    using namespace std;

    class ScmEnv {
        shared_ptr<ScmEnv> parent_env;
        unordered_map<ScmSym, P_ScmObj> binding;

    public:
        ScmEnv(shared_ptr<ScmEnv> penv = nullptr) : parent_env(penv) { }

        P_ScmObj get(const ScmSym & k) {
            auto elem_it = binding.find(k);
            if (elem_it == binding.end()) {
                return nullptr;
            }
            return elem_it->second;
        }

        void set(const ScmSym & k, P_ScmObj & obj) {
            binding[k] = obj;
        }
    };
}

#endif //LLSCHEME_ENVIRONMENT_HPP
