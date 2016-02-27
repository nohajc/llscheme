#include <iostream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "../reader.hpp"

using namespace std;
using namespace llvm;
using namespace llscm;

#define TOKENIZE() 	while (true) { \
	tok = r->nextToken(); \
	if (!tok) break; \
	cout << tok->name << endl; \
}

int main(int argc, char * argv[]) {
	const Token * tok;
	unique_ptr<Reader> r;
	//Reader * pr;

	r = make_unique<StringReader>("(1 -2 3.4 \"str\" sym #t #f null - -. -7.)");
	//pr = new StringReader("(1 -2 3.4 \"str\" sym #t #f null - -. -7.)");
	TOKENIZE();

	cout << "-------------------------------" << endl;

	r = make_unique<FileReader>(cin);
	//pr = new FileReader(cin);
	TOKENIZE();

	return 0;
}
