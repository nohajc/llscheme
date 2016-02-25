#include <cstdint>
#include "types.hpp"

namespace llscm {
	const int32_t ArgsAnyCount = -1;

	ScmCons::~ScmCons() {
		car.reset();
		cdr.reset();
	}

	ScmFunc::~ScmFunc() {
		arg_list.reset();
		body_list.reset();
	}

	ScmCall::~ScmCall() {
		func.reset();
		arg_list.reset();
	}
}
