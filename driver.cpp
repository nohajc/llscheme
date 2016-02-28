#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "parser.hpp"
#include "driver.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	bool compile(unique_ptr<Parser> && p) {
		vector<P_ScmObj> prog = p->NT_Prog();
		if (p->fail()) {
			return false;
		}
		for (auto & e: prog) {
			cout << *e << endl;
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
	if (argc == 2) {
		if (!llscm::compileSourceFile(argv[1])) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
