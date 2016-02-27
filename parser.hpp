#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "types.hpp"
#include "reader.hpp"

namespace llscm {
	using namespace std;

	class ParserException {
		string msg;
	public:
		ParserException(const string & str): msg(str) {}
		const char * what() {
			return msg.c_str();
		}
	};

	class Parser {
		const unique_ptr<Reader>& reader;
		void match(const Token & tok, const Token && expected);
	public:
		Parser(const unique_ptr<Reader>& r): reader(r) {}

		vector<P_ScmObj> NT_Prog();
		P_ScmObj NT_Form();
		P_ScmObj NT_Def();
		P_ScmObj NT_Expr();
		P_ScmObj NT_Atom();
		P_ScmObj NT_List();
		P_ScmObj NT_SymList();
		P_ScmObj NT_BindList();
		P_ScmObj NT_Body();
	};
}
