#include <string>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include "reader.hpp"
#include "debug.hpp"

namespace llscm {
	const char * KwrdNames[] = {
		"(", ")", "#t", "#f", "null",
		"define", "lambda", "quote", "if", "let",
		nullptr
	};

	void Token::deduceType() {
		bool numeric = true;
		bool is_float = false;
		bool is_keyword = false;
		int first_digit_idx = 0;

		if (((name[0] == '\"') && (name[name.length() - 1] == '\"'))
			  || ((name[0] == '\'') && (name[name.length() - 1] == '\''))) {
			t = STR;
			D(cerr << "string token ");
			return;
		}

		if (name[0] == '-') {
			first_digit_idx = 1;
			if (name.length() == 1) {
				goto not_a_number;
			}
		}

		if (name[0] != '-' && !isdigit(name[0])) {
			goto not_a_number;
		}

		for (int i = 1; i < name.length(); ++i) {
			if (!is_float && name[i] == '.' && i > first_digit_idx) {
				is_float = true;
				continue;
			}
			if (!isdigit(name[i])) {
				numeric = false;
				break;
			}
		}

		if (numeric) {
			if (is_float) {
				t = FLOAT;
				float_val = atof(name.c_str());
				D(cerr << "float token [" << float_val << "] ");
				return;
			}
			t = INT;
			int_val = strtoll(name.c_str(), nullptr, 10);
			D(cerr << "int token [" << int_val << "] ");
			return;
		}

		not_a_number:
		int i = 0;
		while (KwrdNames[i]) {
			if (name == KwrdNames[i]) {
				t = KWRD;
				kw = (Keyword) i;
				is_keyword = true;
				D(cerr << "keyword token [" << KwrdNames[(int)kw] << "] ");
			}

			i++;
		}

		if (is_keyword) return;

		t = SYM;
		D(cerr << "symbol token ");
	}

	Reader::Reader() {
		par_left = 0;
	}

	FileReader::FileReader(istream & f) {
		is = &f;
	}

	StringReader::StringReader(const string & str) {
		is = new stringstream(str);
	}

	StringReader::~StringReader() {
		delete is;
	}

	Token * Reader::nextToken() {
		char c;

		if (par_left > 0) {
			par_left--;
			if (tok.t != KWRD || tok.kw != KW_RPAR) {
				tok.name = ")";
				tok.t = KWRD;
				tok.kw = KW_RPAR;
			}
			D(cerr << "keyword token [" << KwrdNames[(int)tok.kw] << "] ");
			return &tok;
		}

		*is >> c;
		if (is->eof()) {
			return nullptr;
		}

		if (c == '(') {
			tok.name = "(";
			tok.t = KWRD;
			tok.kw = KW_LPAR;
			D(cerr << "keyword token [" << KwrdNames[(int)tok.kw] << "] ");
			return &tok;
		}

		if (c == ')') {
			tok.name = ")";
			tok.t = KWRD;
			tok.kw = KW_RPAR;
			D(cerr << "keyword token [" << KwrdNames[(int)tok.kw] << "] ");
			return &tok;
		}

		is->unget();
		is->clear();
		*is >> tok.name;
		while (tok.name[tok.name.length() - 1] == ')') {
			par_left++;
			tok.name.pop_back();
		}
		tok.deduceType();
		return &tok;
	}

	Token * Reader::currToken() {
		return &tok;
	}
}
