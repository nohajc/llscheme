#include <string>
#include <iostream>
#include "../include/reader.hpp"
#include "../include/debug.hpp"

namespace llscm {
	const char * KwrdNames[] = {
		"(", ")", "#t", "#f", "null",
		"define", "lambda", "quote", "if", "let",
		nullptr
	};

	Token::Token(Keyword k): kw(k) {
		t = KWRD;
		name = KwrdNames[(int) k];
	}

	bool operator==(const Token & lhs, const Token & rhs) {
		switch (lhs.t) {
		case INT:
			return lhs.t == rhs.t && lhs.int_val == rhs.int_val;
		case FLOAT:
			return lhs.t == rhs.t && lhs.float_val == rhs.float_val;
		case KWRD:
			return lhs.t == rhs.t && lhs.kw == rhs.kw;
		default:
			return lhs.t == rhs.t;
		}
	}

	bool operator!=(const Token & lhs, const Token & rhs) {
		return !(lhs == rhs);
	}

	void Token::deduceType() {
		bool numeric = true;
		bool is_float = false;
		bool is_keyword = false;
		size_t first_digit_idx = 0;

		if (name[0] == '-') {
			first_digit_idx = 1;
			if (name.length() == 1) {
				goto not_a_number;
			}
		}

		if (name[0] != '-' && !isdigit(name[0])) {
			goto not_a_number;
		}

		for (size_t i = 1; i < name.length(); ++i) {
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
				//D(cerr << "float token [" << float_val << "] ");
				return;
			}
			t = INT;
			int_val = strtoll(name.c_str(), nullptr, 10);
			//D(cerr << "int token [" << int_val << "] ");
			return;
		}

		not_a_number:
		int i = 0;
		while (KwrdNames[i]) {
			if (name == KwrdNames[i]) {
				t = KWRD;
				kw = (Keyword) i;
				is_keyword = true;
				//D(cerr << "keyword token [" << KwrdNames[(int)kw] << "] ");
			}

			i++;
		}

		if (is_keyword) return;

		t = SYM;
		//D(cerr << "symbol token ");
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

	const Token * Reader::nextToken() {
		char c;
		tok.name = "";

		do {
			is->get(c);
			if (is->eof()) {
				return nullptr;
			}
		} while (isspace(c));

		switch (c) {
			case '(':
				tok.name = "(";
				tok.t = KWRD;
				tok.kw = KW_LPAR;
				return &tok;
			case ')':
				tok.name = ")";
				tok.t = KWRD;
				tok.kw = KW_RPAR;
				return &tok;
			case '\"':
				goto read_string;
			default:;
		}
		// Read literal
		do {
			//D(cerr << "DEBUG: " << c << endl);
			tok.name += c;
			is->get(c);
			if (is->eof()) {
				break;
			}
		} while (c != ')' && !isspace(c));
		is->unget();
		is->clear();
		tok.deduceType();
		return &tok;

		read_string:
		do {
			tok.name += c;
			is->get(c);
			if (is->eof()) {
				is->clear();
				tok.t = ERR;
				D(cerr << "ERR:" << tok.name << " ");
				error("Reached EOF while parsing a string.");
				return &tok;
			}
		} while (c != '\"');
		tok.name += c;
		tok.t = STR;
		D(cerr << "STR:" << tok.name << " ");
		return &tok;
	}

	const Token * Reader::currToken() {
		if (is->eof()) {
			return nullptr;
		}
		return &tok;
	}

	void Reader::error(const string & msg) {
		cout << "Error: " << msg << endl;
	}

}
