#include <llvm/ADT/STLExtras.h>
#include "../include/environment.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;

    shared_ptr<ScmEnv> createGlobalEnvironment() {
        shared_ptr<ScmEnv> env = make_shared<ScmEnv>();
        //unique_ptr<ScmObj> str = make_unique<ScmStr>("hello world");
        shared_ptr<ScmObj> sh_str = make_shared<ScmStr>("spam");

        //env->set(ScmSym("str"), str);
        env->set(ScmSym("sh_str"), sh_str);


        return env;
    }
}
