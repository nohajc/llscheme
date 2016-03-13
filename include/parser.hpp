#ifndef LLSCHEME_PARSER_HPP
#define LLSCHEME_PARSER_HPP

#include <memory>
#include <list>
#include <llvm/ADT/STLExtras.h>
#include "ast.hpp"
#include "reader.hpp"

namespace llscm {
	using namespace std;

	/*class ParserException {
		string msg;
	public:
		ParserException(const string & str): msg(str) {}
		const char * what() {
			return msg.c_str();
		}
	};*/

	class Parser {
		const unique_ptr<Reader>& reader;
		bool err_flag;

		bool match(const Token * tok, const Token && expected);
		void error(const string & msg);
	public:
		Parser(const unique_ptr<Reader>& r): reader(r) {
			err_flag = false;
		}
		bool fail() {
			return err_flag;
		}

		ScmProg NT_Prog();
		P_ScmObj NT_Form();
		P_ScmObj NT_Def();
		P_ScmObj NT_CallOrSyntax();
		P_ScmObj NT_Expr();
		P_ScmObj NT_Data();
		P_ScmObj NT_Atom(bool quoted);
		P_ScmObj NT_List();
		P_ScmObj NT_SymList();
		P_ScmObj NT_BindList();
		P_ScmObj NT_Body();
	};
}

#endif //LLSCHEME_PARSER_HPP