#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "../include/driver.hpp"
#include "../include/parser.hpp"
#include "../include/environment.hpp"
#include "../include/codegen.hpp"
#include "../include/compiler.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	int argc;
	char ** argv;

	bool compile(unique_ptr<Parser> && p) {
		ScmProg prog = p->NT_Prog();
		shared_ptr<ScmEnv> env = createGlobalEnvironment(prog);
		if (p->fail()) {
			return false;
		}
		for (auto & e: prog) {
			cout << *e;
			e = e->CT_Eval(env);
			if (env->fail()) {
				return false;
			}
		}

		for (auto & e: prog) {
			e->printSrc(cerr);
			cerr << endl;
		}

		ScmCodeGen cg(getGlobalContext(), &prog);
		cg.dump();

		ScmCompiler scm_compiler(cg.getModule(), cg.getContext());
		int err_code = scm_compiler.run();

		return !err_code;
	}

	bool compileSourceFile(const char * fname) {
		bool st;
		if (!strcmp(fname, "-")) {
			unique_ptr<Reader> r = make_unique<FileReader>(cin);
			st = compile(make_unique<Parser>(r));
		}
		else {
			fstream fs(fname, fstream::in);
			if (fs.fail()) {
				cerr << "Cannot open file " << fname << "." << endl;
				return false;
			}

			unique_ptr<Reader> r = make_unique<FileReader>(fs);
			st = compile(make_unique<Parser>(r));

			fs.close();
		}
		return st;
	}

	bool compileString(const char * str) {
		unique_ptr<Reader> r = make_unique<StringReader>(str);
		return compile(make_unique<Parser>(r));
	}
}
