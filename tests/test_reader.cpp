#include <iostream>
#include "../reader.hpp"

using namespace llscm;
using namespace std;

#define TOKENIZE() 	while (true) { \
	tok = r->nextToken(); \
	if (!tok) break; \
	cout << tok->name << endl; \
}

int main(int argc, char * argv[]) {
	Token * tok;
	Reader * r;

	r = new StringReader("(1 -2 3.4 \"str\" sym #t #f null - -. -7.)");
	TOKENIZE();
	delete r;

	cout << "-------------------------------" << endl;

	r = new FileReader(cin);
	TOKENIZE();
	delete r;

	return 0;
}
