#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "../include/driver.hpp"
#include "../include/parser.hpp"
#include "../include/environment.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	bool compile(unique_ptr<Parser> && p) {
		ScmProg prog = p->NT_Prog();
		shared_ptr<ScmEnv> env = createGlobalEnvironment(prog);
		if (p->fail()) {
			return false;
		}
		for (auto & e: prog) {
			cout << *e;
			e->printSrc(cerr);
			cerr << endl;
			//e->CT_Eval(env);
		}

		return true;
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

int main(int argc, char * argv[]) {
	if (argc >= 2) {
		if (!strcmp(argv[1], "-s")) {
			if (argc != 3) return EXIT_SUCCESS;
			if (!llscm::compileString(argv[2])) {
				return EXIT_FAILURE;
			}
		}
		else if (!llscm::compileSourceFile(argv[1])) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
