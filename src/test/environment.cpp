#include "../../include/environment.hpp"
#include <UnitTest++/UnitTest++.h>

using namespace llscm;

SUITE(ScmEnvTest) {
        class ScmEnvFixture {
        public:
            ScmProg prog;
            P_ScmEnv env = createGlobalEnvironment(prog);
        };

        TEST_FIXTURE(ScmEnvFixture, LambdaIDs) {
            CHECK_EQUAL("lambda0", env->getUniqID("lambda"));
            CHECK_EQUAL("lambda1", env->getUniqID("lambda"));
            CHECK_EQUAL("lambda2", env->getUniqID("lambda"));
        }
}
