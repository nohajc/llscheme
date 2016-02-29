#ifndef LLSCHEME_READER_HPP
#define LLSCHEME_READER_HPP

#include <iostream>
#include <sstream>
#include <string>

namespace llscm {
	using namespace std;

	enum TokenType {
		INT, FLOAT, STR, SYM, KWRD, ERR
	};

	enum Keyword {
		KW_LPAR, KW_RPAR, KW_TRUE, KW_FALSE, KW_NULL,
		KW_DEFINE, KW_LAMBDA, KW_QUOTE, KW_IF, KW_LET
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
		int par_left;
	public:
		const Token * nextToken();
		const Token * currToken();

		Reader();
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
}

#endif //LLSCHEME_READER_HPP