#include <iostream>
#include "../reader.hpp"

using namespace llscm;
using namespace std;

#define TOKENIZE() 	while (true) { \
	tok = r->nextToken(); \
	if (!tok) break; \
	cout << tok << endl; \
}

int main(int argc, char * argv[]) {
	const char * tok;
	Reader * r;

	r = new StringReader("(1 2 3)");
	TOKENIZE();
	delete r;

	r = new FileReader(cin);
	TOKENIZE();
	delete r;

	return 0;
}