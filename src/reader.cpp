#include <string>
#include <cstring>
#include <iostream>
#include <llvm/ADT/STLExtras.h>
#include "../include/reader.hpp"
#include "../include/debug.hpp"
#include "../include/runtime/internal.hpp"

namespace llscm {
	using namespace llvm;

	const char * KwrdNames[] = {
		"(", ")", "#t", "#f", "null",
		"define", "lambda", "quote", "if", "let", "\'",
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

	/*Reader::Reader() {
		par_left = 0;
	}*/

	FileReader::FileReader(istream & f) {
		is = &f;
	}

	StringReader::StringReader(const string & str) {
		is = new stringstream(str);
	}

	StringReader::~StringReader() {
		delete is;
	}

	char Reader::skipSpaces() {
		char c;
		do {
			is->get(c);
			if (is->eof()) {
				return 0;
			}
		} while (isspace(c));

		return c;
	}

	const Token * Reader::nextToken() {
		char c;
		bool esc = false;
		tok.name = "";

		c = skipSpaces();
		if (is->eof()) {
			return nullptr;
		}

		while (c == ';') {
			// Skip comment
			do {
				is->get(c);
				if (is->eof()) {
					return nullptr;
				}
			} while(c != '\n');

			c = skipSpaces();
			if (is->eof()) {
				return nullptr;
			}
		}

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
			case '\'':
				tok.name = "\'";
				tok.t = KWRD;
				tok.kw = KW_QUCHAR;
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
		while (true) {
			is->get(c);
			if (is->eof()) {
				is->clear();
				tok.t = ERR;
				D(cerr << "ERR:" << tok.name << " ");
				error("Reached EOF while parsing a string.");
				return &tok;
			}

			if (!esc) {
				if (c == '\\') {
					esc = true;
				}
				else if (c == '\"') {
					break;
				}
				else {
					tok.name += c;
				}
			}
			else {
				// Escape sequences
				switch (c) {
					case 'n':
						tok.name += '\n';
						break;
					case 't':
						tok.name += '\t';
						break;
					case 'b':
						tok.name += '\b';
						break;
					case 'r':
						tok.name += '\r';
						break;
					case '\\':
						tok.name += '\\';
					case '\"':
						tok.name += '\"';
					default:
						tok.name += '\\' + c;
				}
				esc = false;
			}
		};

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

	using namespace runtime;

	ListReader::ListReader(scm_ptr_t expr) {
		st.push_back(expr);
		eof = false;
		lstend_mark = make_unique<scm_type_t>();
		lstend_mark->tag = -1;
	}

	const Token * ListReader::nextToken() {
		if (st.empty()) {
			eof = true;
			return nullptr;
		}

		scm_ptr_t obj = st.back();
		st.pop_back();

		if (obj.asType == lstend_mark.get()) {
			tok.t = KWRD;
			tok.name = ")";
			tok.kw = KW_RPAR;

			return &tok;
		}

		switch(obj->tag) {
			case S_FALSE: {
				tok.t = KWRD;
				tok.name = "#f";
				tok.kw = KW_FALSE;

				return &tok;
			}
			case S_TRUE: {
				tok.t = KWRD;
				tok.name = "#t";
				tok.kw = KW_TRUE;

				return &tok;
			}
			case S_NIL: {
				tok.t = KWRD;
				tok.name = "null";
				tok.kw = KW_NULL;

				return &tok;
			}
			case S_INT: {
				tok.t = INT;
				tok.name = "";
				tok.int_val = obj.asInt->value;

				return &tok;
			}
			case S_FLOAT: {
				tok.t = FLOAT;
				tok.name = "";
				tok.float_val = obj.asFloat->value;

				return &tok;
			}
			case S_STR: {
				tok.t = STR;
				tok.name = obj.asStr->str;

				return &tok;
			}
			case S_SYM: {
				if (!strcmp(obj.asSym->sym, "define")) {
					tok.t = KWRD;
					tok.name = obj.asSym->sym;
					tok.kw = KW_DEFINE;

					return &tok;
				}
				if (!strcmp(obj.asSym->sym, "lambda")) {
					tok.t = KWRD;
					tok.name = obj.asSym->sym;
					tok.kw = KW_LAMBDA;

					return &tok;
				}
				if (!strcmp(obj.asSym->sym, "quote")) {
					tok.t = KWRD;
					tok.name = obj.asSym->sym;
					tok.kw = KW_QUOTE;

					return &tok;
				}
				if (!strcmp(obj.asSym->sym, "if")) {
					tok.t = KWRD;
					tok.name = obj.asSym->sym;
					tok.kw = KW_IF;

					return &tok;
				}
				if (!strcmp(obj.asSym->sym, "let")) {
					tok.t = KWRD;
					tok.name = obj.asSym->sym;
					tok.kw = KW_LET;

					return &tok;
				}
				tok.t = SYM;
				tok.name = obj.asSym->sym;

				return &tok;
			}
			case S_CONS: {
				D(cerr << "ListReader: reading cons." << endl);
				vector<scm_ptr_t> lst_elems;
				list_foreach(obj, [&lst_elems](scm_ptr_t elem) {
					D(cerr << "ListReader: pushing list elem." << endl);
					lst_elems.push_back(elem.asCons->car);
				});
				// Push special list end mark
				st.push_back(lstend_mark.get());
				// Push list elements in reverse order
				st.insert(st.end(), lst_elems.rbegin(), lst_elems.rend());
				tok.t = KWRD;
				tok.name = "(";
				tok.kw = KW_LPAR;

				return &tok;
			}
			default:
				tok.t = ERR;
				error("Invalid token in quoted expression.");
				return &tok;
		}
	}

	const Token * ListReader::currToken() {
		if (eof) {
			return nullptr;
		}
		return &tok;
	}

}
