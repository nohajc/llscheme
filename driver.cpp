#include <cstdlib>
#include <iostream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "parser.hpp"
#include "driver.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	void compileSourceFile(const char * fname) {

	}

	void compileString(const char * str) {
		unique_ptr<Reader> r = make_unique<StringReader>(str);
		unique_ptr<Parser> p = make_unique<Parser>(r);

		try {
			vector<P_ScmObj> prog = p->NT_Prog();
			for (auto & e: prog) {
				cout << *e << endl;
			}
		}
		catch (ParserException & e) {
			cout << e.what() << endl;
		}
	}
}

int main(int argc, char * argv[]) {
	if (argc == 2) {
		llscm::compileString(argv[1]);
	}
	return EXIT_SUCCESS;
}
