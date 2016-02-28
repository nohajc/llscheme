#include <iostream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "parser.hpp"
#include "debug.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	void Parser::match(const Token & tok, const Token && expected) {
		if (tok != expected) {
			stringstream ss;
			ss << "Expected token \"" << expected.name << "\".";
			throw ParserException(ss.str());
		}
	}

	/*
	 * prog = form { form }
	 */
	vector<P_ScmObj> Parser::NT_Prog() {
		const Token * tok;
		vector<P_ScmObj> prog;
		bool first = true;

		D(cout << "NT_Prog: " << endl);

		while (true) {
			tok = reader->nextToken();

			if (!tok) {
				if (first) throw ParserException("Program is empty.");
				else break;
			}
			D(cout << tok->name << endl);
			P_ScmObj form = NT_Form();
			prog.push_back(move(form));
			first = false;
		}

		return prog;
	}

	/*
	 * form = "(" def ")" | expr
	 * def = "define" sym expr ...
	 * expr = atom | ( "(" list ")" )
	 */
	P_ScmObj Parser::NT_Form() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cout << "NT_Form: " << endl);

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();
			if (!tok) {
				throw ParserException("Reached EOF while parsing a list.");
			}
			D(cout << tok->name << endl);
			if (tok->t == KWRD && (tok->kw == KW_DEFINE || tok->kw == KW_LET)) {
				// Current token is "define" or "let"
				obj = NT_Def();
			}
			else {
				// Current token is the first token of list
				obj = NT_List();
			}
			match(*reader->currToken(), Token(KW_RPAR));
			return obj;
		}
		// Anything other than "(" must be an atom
		return NT_Atom();
	}

	/*
	 * def = "define" sym expr
	 *		 | "define" "(" sym symlist ")" body
	 *		 | "let" "(" bindlist ")" body
	 */
	P_ScmObj Parser::NT_Def() {
		const Token * tok = reader->currToken();
		P_ScmObj name, lst, expr;

		D(cout << "NT_Def: " << endl);

		if (tok->kw == KW_DEFINE) {
			tok = reader->nextToken();
			if (!tok) {
				throw ParserException("Reached EOF while parsing a definition.");
			}
			D(cout << tok->name << endl);

			if (tok->t == KWRD && tok->kw == KW_LPAR) {
				// Function definition
				tok = reader->nextToken();
				if (tok->t != SYM)
					throw ParserException("Missing function name in definition.");
				name = make_unique<ScmSym>(tok->name);
				reader->nextToken();
				lst = NT_SymList();
				match(*reader->currToken(), Token(KW_RPAR));
				reader->nextToken();

				expr = NT_Body();
				reader->nextToken();
				return make_unique<ScmDefineFuncSyntax>(move(name), move(lst), move(expr));
			}
			if (tok->t != SYM)
				throw ParserException("Expected symbol as first argument of define.");
			name = make_unique<ScmSym>(tok->name);
			tok = reader->nextToken();
			D(cout << tok->name << endl);

			expr = NT_Expr();
			tok = reader->nextToken();
			D(cout << tok->name << endl);

			return make_unique<ScmDefineVarSyntax>(move(name), move(expr));
		}
		else { // tok->kw == KW_LET
			match(*reader->nextToken(), Token(KW_LPAR));
			tok = reader->nextToken();
			lst = NT_BindList();
			match(*reader->currToken(), Token(KW_RPAR));
			reader->nextToken();

			expr = NT_Body();
			reader->nextToken();
			return make_unique<ScmLetSyntax>(move(lst), move(expr));
		}
		//return nullptr;
	}

	/*
	 * expr = atom | ( "(" list ")" )
	 */
	P_ScmObj Parser::NT_Expr() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cout << "NT_Expr: " << endl);

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();
			obj = NT_List();
			match(*reader->currToken(), Token(KW_RPAR));
			return obj;
		}
		obj = NT_Atom();
		/*tok = reader->nextToken();
		D(cout << tok->name << endl);*/
		return obj;
	}

	/*
	 * atom = str | sym | int | float | true | false | null
	 */
	P_ScmObj Parser::NT_Atom() {
		const Token * tok = reader->currToken();

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

		if (tok->t != KWRD) throw ParserException("Invalid token for an atom.");
		switch (tok->kw) {
		case KW_TRUE:
			return make_unique<ScmTrue>();
		case KW_FALSE:
			return make_unique<ScmFalse>();
		case KW_NULL:
			return make_unique<ScmNull>();
		default:
			throw ParserException("Invalid token for an atom.");
		}
	}

	/*
	 * list = { expr }
	 */
	P_ScmObj Parser::NT_List() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		obj = NT_Expr();
		reader->nextToken();
		return make_unique<ScmCons>(move(obj), NT_List());
	}

	/*
	 * symlist = { sym }
	 */
	P_ScmObj Parser::NT_SymList() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		if (tok->t != SYM)
				throw ParserException("Invalid expression in argument list. Only symbols are allowed.");
		obj = make_unique<ScmSym>(tok->name);
		reader->nextToken();
		return make_unique<ScmCons>(move(obj), NT_SymList());
	}

	/*
	 * bindlist = { "(" sym expr ")" }
	 */
	P_ScmObj Parser::NT_BindList() {
		const Token * tok = reader->currToken();
		vector<P_ScmObj> vec;

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		match(*reader->currToken(), Token(KW_LPAR));
		tok = reader->nextToken();
		if (tok->t != SYM)
			throw ParserException("First element of binding list must be symbol.");

		vec.push_back(make_unique<ScmSym>(tok->name));
		reader->nextToken();
		vec.push_back(NT_Expr());
		match(*reader->nextToken(), Token(KW_RPAR));
		reader->nextToken();

		return make_unique<ScmCons>(makeScmList(move(vec)), NT_BindList());
	}

	/*
	 * body = { "(" def ")" } expr { expr }
	 */
	P_ScmObj Parser::NT_Body() {
		vector<P_ScmObj> lst;
		const Token * tok = reader->currToken();
		P_ScmObj obj;
		bool parsing_defs = true;

		while (parsing_defs) {
			if (tok->t == KWRD && tok->kw == KW_RPAR) {
				throw ParserException("Missing expression in function body.");
			}
			obj = NT_Form();
			if (dynamic_cast<ScmDefineSyntax*>(obj.get()) == nullptr) {
				parsing_defs = false;
			}
			lst.push_back(move(obj));
			tok = reader->nextToken();
		}

		while (tok->t != KWRD || tok->kw != KW_RPAR) {
			lst.push_back(NT_Expr());
			tok = reader->nextToken();
		}

		return makeScmList(move(lst));
	}
};
