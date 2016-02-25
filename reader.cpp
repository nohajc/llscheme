#include <string>
#include "reader.hpp"

namespace llscm {
	FileReader::FileReader(istream & f) {
		is = &f;
	}

	StringReader::StringReader(const string & str) {
		is = new stringstream(str);
	}

	StringReader::~StringReader() {
		delete is;
	}

	const char * Reader::nextToken() {
		char c;

		*is >> c;
		if (is->eof()) {
			return nullptr;
		}

		if (c == '(' || c == ')') {
			return string(&c, 1).c_str();
		}
		is->unget();
		is->clear();
		*is >> tok;
		if (tok[tok.length() - 1] == ')') {
			is->unget();
			is->clear();
			tok.pop_back();
		}
		return tok.c_str();
	}

	const char * Reader::currToken() {
		return tok.c_str();
	}
}
