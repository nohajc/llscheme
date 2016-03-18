#ifndef LLSCHEME_DRIVER_HPP
#define LLSCHEME_DRIVER_HPP

//#include <llvm/ADT/StringRef.h>

namespace llscm {
	using namespace llvm;

	extern int argc;
	extern char ** argv;

	bool compileSourceFile(const char * fname);
	bool compileString(const char * str);
}

#endif //LLSCHEME_DRIVER_HPP