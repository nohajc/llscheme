#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "parser.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	/*
	 * prog = form { form }
	 */
	vector<P_ScmObj> Parser::NT_Prog() {
		Token * tok;
		vector<P_ScmObj> prog;

		while (true) {
			tok = reader->nextToken();
			// TODO: if (!tok) REPORT ERROR
			P_ScmObj form = NT_Form();
			if (!form) break;
			prog.push_back(move(form));
		}

		return prog;
	}

	/*
	 * form = def | expr
	 * def = "(" "define" sym expr ")" ...
	 * expr = atom | ( "(" list ")" )
	 */
	P_ScmObj Parser::NT_Form() {
		Token * tok = reader->currToken();

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();
			// TODO: if (!tok) REPORT ERROR
			if (tok->t == KWRD && tok->kw == KW_DEFINE) {
				// Current token is "define"
				return NT_Def();
			}
			// Current token is the first token of list
			return NT_List();
		}
		// Anything other than "(" must be an atom
		return NT_Atom();
	}

	/*
	 * def = "(" "define" sym expr ")"
	 *		 | "(" "define" "(" symlist ")" body ")"
	 *		 | "(" "let" "(" bindlist ")" body ")"
	 */
	P_ScmObj Parser::NT_Def() {
		return nullptr;
	}

	/*
	 * expr = atom | ( "(" list ")" )
	 */
	P_ScmObj Parser::NT_Expr() {
		Token * tok = reader->currToken();

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();
			return NT_List();
		}
		return NT_Atom();
	}

	/*
	 * atom = str | sym | int | float | true | false | null
	 */
	P_ScmObj Parser::NT_Atom() {
		Token * tok = reader->currToken();

		switch (tok->t) {
		case STR:
			return make_unique<ScmStr>(tok->name);
		case SYM:
			return make_unique<ScmSym>(tok->name);
		case INT:
			return make_unique<ScmInt>(tok->int_val);
		case FLOAT:
			return make_unique<ScmFloat>(tok->float_val);
		default:;
		}

		// TODO: if (tok->t != KWRD) REPORT ERROR
		switch (tok->kw) {
		case KW_TRUE:
			return make_unique<ScmTrue>();
		case KW_FALSE:
			return make_unique<ScmFalse>();
		case KW_NULL:
			return make_unique<ScmNull>();
		default:
			return nullptr; // TODO: REPORT ERROR (this would look like EOF)
		}
	}

	/*
	 * list = { expr }
	 */
	P_ScmObj Parser::NT_List() {
		// TODO: special cases: lambda, quote, if
		return nullptr;
	}

	/*
	 * symlist = { sym }
	 */
	P_ScmObj Parser::NT_SymList() {
		return nullptr;
	}

	/*
	 * bindlist = { "(" sym expr ")" }
	 */
	P_ScmObj Parser::NT_BindList() {
		return nullptr;
	}

	/*
	 * body = { def } expr { expr }
	 */
	P_ScmObj Parser::NT_Body() {
		return nullptr;
	}
};
