#ifndef LLSCHEME_READER_HPP
#define LLSCHEME_READER_HPP

#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include "runtime.h"

namespace llscm {
	using namespace std;

	enum TokenType {
		INT, FLOAT, STR, SYM, KWRD, ERR
	};

	enum Keyword {
		KW_LPAR, KW_RPAR, KW_TRUE, KW_FALSE, KW_NULL,
		KW_DEFINE, KW_LAMBDA, KW_QUOTE, KW_IF, KW_LET, KW_QUCHAR
	};

	extern const char * KwrdNames[];

	struct Token {
		TokenType t;
		string name;

		union {
			int64_t int_val;
			double float_val;
			Keyword kw;
		};

		Token() {}
		Token(Keyword k);
		friend bool operator==(const Token & lhs, const Token & rhs);
		friend bool operator!=(const Token & lhs, const Token & rhs);
		void deduceType();
	};

	class Reader {
	protected:
		Token tok;
		istream * is;
		char skipSpaces();
	public:
		virtual const Token * nextToken();
		virtual const Token * currToken();
		void error(const string & msg);

		//Reader();
		virtual ~Reader() {};
	};

	class FileReader: public Reader {
	public:
		FileReader(istream & f);
	};

	class StringReader: public Reader {
	public:
		StringReader(const string & str);
		virtual ~StringReader();
	};

	// Used when calling compiler through eval function at runtime
	class ListReader: public Reader {
		vector<runtime::scm_ptr_t> st; // Using vector as stack
		unique_ptr<runtime::scm_type_t> lstend_mark;
		unique_ptr<runtime::scm_type_t> quote_mark;
		bool eof;
		bool quoted;
	public:
		virtual const Token * nextToken();
		virtual const Token * currToken();

		ListReader(runtime::scm_ptr_t expr);
	};
}

#endif //LLSCHEME_READER_HPP