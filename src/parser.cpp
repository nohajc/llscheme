#include <iostream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "../include/parser.hpp"
#include "../include/debug.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	bool Parser::match(const Token * tok, const Token && expected) {
		if (!tok || *tok != expected) {
			stringstream ss;
			ss << "Expected token \"" << expected.name << "\".";
			error(ss.str());
			return false;
		}
		return true;
	}

	void Parser::error(const string & msg) {
		cerr << "Error: " << msg << endl;
		err_flag = true;
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
				if (first) {
					error("Program is empty.");
					return prog;
				}
				else break;
			}
			//D(cout << tok->name << endl);
			P_ScmObj form = NT_Form();
			if (fail()) break;
			prog.push_back(move(form));
			first = false;
		}

		return prog;
	}

	/*
	 * form = "(" def ")" | expr
	 * def = "define" sym expr ...
	 * expr = atom | "(" callsyn ")"
	 */
	P_ScmObj Parser::NT_Form() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cout << "NT_Form: " << endl);
		D(cout << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();
			if (!tok) {
				error("Reached EOF while parsing a list.");
				return nullptr;
			}
			//D(cout << tok->name << endl);
			if (tok->t == KWRD && (tok->kw == KW_DEFINE || tok->kw == KW_LET)) {
				// Current token is "define" or "let"
				obj = NT_Def();
				if (fail()) return nullptr;
			}
			else {
				obj = NT_CallOrSyntax();
				if (fail()) return nullptr;
			}
			if (!match(reader->currToken(), Token(KW_RPAR))) {
				return nullptr;
			}
			return obj;
		}
		// Anything other than "(" must be an atom
		return NT_Atom(false);
	}

	/*
	 * callsyn = "lambda" "(" symlist ")" body
	 *		   | "quote" data
	 *		   | "if" expr expr expr
	 *		   | expr { expr }
	 */
	P_ScmObj Parser::NT_CallOrSyntax() {
		const Token * tok = reader->currToken();
		P_ScmObj expr, ce, te, ee;
		vector<P_ScmObj> lst;

		if (tok->t == KWRD) {
			switch (tok->kw) {
				case KW_LAMBDA:
					if (!match(reader->nextToken(), Token(KW_LPAR))) {
						return nullptr;
					}
					reader->nextToken();
					expr = NT_SymList();
					if (fail()) return nullptr;

					if (!match(reader->currToken(), Token(KW_RPAR))) {
						return nullptr;
					}

					reader->nextToken();
					return make_unique<ScmLambdaSyntax>(move(expr), NT_Body());
				case KW_QUOTE:
					reader->nextToken();
					expr = NT_Data();
					if (fail()) return nullptr;
					reader->nextToken();
					return make_unique<ScmQuoteSyntax>(move(expr));
					break;
				case KW_IF:
					reader->nextToken();
					ce = NT_Expr();
					if (fail()) return nullptr;
					reader->nextToken();
					te = NT_Expr();
					if (fail()) return nullptr;
					reader->nextToken();
					ee =NT_Expr();
					if (fail()) return nullptr;
					reader->nextToken();
					return make_unique<ScmIfSyntax>(move(ce), move(te), move(ee));
				default:
					error("Unexpected keyword at first list position.");
					return nullptr;
			}
		}
		// else: function call
		expr = NT_Expr(); // function
		if (fail()) return nullptr;
		tok = reader->nextToken();
		do {
			if (!tok) {
				error("Reached EOF while parsing function call.");
				return nullptr;
			}
			if (tok->t == KWRD && tok->kw == KW_RPAR) {
				return make_unique<ScmCall>(move(expr), makeScmList(move(lst)));
			}
			lst.push_back(NT_Expr());
			if (fail()) return nullptr;
			tok = reader->nextToken();
		} while (true);
	}

	/*
	 * def = "define" sym expr
	 *		 | "define" "(" sym symlist ")" body
	 *		 | "let" "(" bindlist ")" body
	 */
	P_ScmObj Parser::NT_Def() {
		const Token * tok = reader->currToken();
		P_ScmObj name, lst, expr;

		if (tok->kw == KW_DEFINE) {
			D(cout << "NT_Def: " << endl);

			tok = reader->nextToken();
			if (!tok) {
				error("Reached EOF while parsing a definition.");
				return nullptr;
			}
			D(cout << tok->name << endl);

			if (tok->t == KWRD && tok->kw == KW_LPAR) {
				// Function definition
				tok = reader->nextToken();
				if (!tok || tok->t != SYM) {
					error("Missing function name in definition.");
					return nullptr;
				}
				name = make_unique<ScmSym>(tok->name);
				reader->nextToken();
				lst = NT_SymList();
				if (fail()) return nullptr;

				if (!match(reader->currToken(), Token(KW_RPAR))) {
					return nullptr;
				}
				reader->nextToken();

				return make_unique<ScmDefineFuncSyntax>(move(name), move(lst), NT_Body());
			}
			if (tok->t != SYM) {
				error("Expected symbol as first argument of define.");
				return nullptr;
			}
			name = make_unique<ScmSym>(tok->name);
			tok = reader->nextToken();
			//D(cout << tok->name << endl);
			if (tok && tok->t == KWRD && tok->kw == KW_RPAR) {
				error("Missing expression in variable definition.");
				return nullptr;
			}

			expr = NT_Expr();
			if (fail()) return nullptr;
			reader->nextToken();
			//D(cout << tok->name << endl);

			return make_unique<ScmDefineVarSyntax>(move(name), move(expr));
		}
		else { // tok->kw == KW_LET
			D(cout << "NT_Let: " << endl);

			if (!match(reader->nextToken(), Token(KW_LPAR))) {
				return nullptr;
			}
			reader->nextToken();
			//D(cout << tok->name << endl);

			lst = NT_BindList();
			if (fail()) return nullptr;
			if (!match(reader->currToken(), Token(KW_RPAR))) {
				return nullptr;
			}
			reader->nextToken();
			//D(cout << tok->name << endl);

			return make_unique<ScmLetSyntax>(move(lst), NT_Body());
		}
		//return nullptr;
	}

	/*
	 * expr = atom | ( "(" callsyn ")" )
	 */
	P_ScmObj Parser::NT_Expr() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cout << "NT_Expr: " << endl);

		if (!tok) {
			error("Expected expression.");
			return nullptr;
		}

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			reader->nextToken();
			obj = NT_CallOrSyntax();
			if (fail()) return nullptr;

			if (!match(reader->currToken(), Token(KW_RPAR))) {
				return nullptr;
			}
			return obj;
		}
		D(cout << tok->name << endl);
		return NT_Atom(false);
	}

	/*
	 * data = atom | ( "(" list ")" )
	 */
	P_ScmObj Parser::NT_Data() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cout << "NT_Expr: " << endl);

		if (!tok) {
			error("Expected atom or list.");
			return nullptr;
		}

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			reader->nextToken();
			obj = NT_List();
			if (fail()) return nullptr;

			if (!match(reader->currToken(), Token(KW_RPAR))) {
				return nullptr;
			}
			return obj;
		}
		D(cout << tok->name << endl);
		return NT_Atom(true);
	}

	/*
	 * atom = str | sym | int | float | true | false | null
	 */
	P_ScmObj Parser::NT_Atom(bool quoted) {
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

		if (tok->t != KWRD) {
			error("Invalid token for an atom.");
			return nullptr;
		}
		switch (tok->kw) {
			case KW_TRUE:
				return make_unique<ScmTrue>();
			case KW_FALSE:
				return make_unique<ScmFalse>();
			case KW_NULL:
				return make_unique<ScmNull>();
			default:
				if (!quoted) error("Invalid token for an atom.");
		}
		return make_unique<ScmSym>(tok->name);
	}

	/*
	 * list = { data }
	 */
	P_ScmObj Parser::NT_List() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cout << "NT_List: " << endl);
		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

		D(cout << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		obj = NT_Data();
		if (fail()) return nullptr;
		reader->nextToken();
		return make_unique<ScmCons>(move(obj), NT_List());
	}

	/*
	 * symlist = { sym }
	 */
	P_ScmObj Parser::NT_SymList() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		if (tok->t != SYM) {
			error("Invalid expression in argument list. Only symbols are allowed.");
			return nullptr;
		}
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

		D(cout << "NT_BindList" << endl);
		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

		D(cout << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		if (!match(reader->currToken(), Token(KW_LPAR))) {
			return nullptr;
		}
		tok = reader->nextToken();
		//D(cout << tok->name << endl);

		if (!tok || tok->t != SYM) {
			error("First element of binding list must be a symbol.");
			return nullptr;
		}

		vec.push_back(make_unique<ScmSym>(tok->name));
		tok = reader->nextToken();

		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			error("Binding list must have exactly two elements: id, expression.");
			return nullptr;
		}
		//D(cout << tok->name << endl);

		vec.push_back(NT_Expr());
		if (!match(reader->nextToken(), Token(KW_RPAR))) {
			return nullptr;
		}
		reader->nextToken();
		//D(cout << tok->name << endl);

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
		bool no_expr = true;

		D(cout << "NT_Body" << endl);

		while (parsing_defs) {
			if (!tok) {
				error("Reached EOF while parsing a body.");
				return nullptr;
			}
			D(cout << tok->name << endl);

			if (tok->t == KWRD && tok->kw == KW_RPAR) {
				if (no_expr) {
					error("Missing expression in a body.");
					return nullptr;
				}
				break;
			}
			obj = NT_Form();
			if (dynamic_cast<ScmDefineSyntax*>(obj.get()) == nullptr) {
				parsing_defs = false;
			}
			if (dynamic_cast<ScmLetSyntax*>(obj.get()) != nullptr) {
				no_expr = false;
			}
			lst.push_back(move(obj));
			tok = reader->nextToken();
		}

		do {
			if (!tok) {
				error("Reached EOF while parsing a body.");
				return nullptr;
			}
			if (tok->t == KWRD && tok->kw == KW_RPAR) {
				return makeScmList(move(lst));
			}
			lst.push_back(NT_Expr());
			if (fail()) return nullptr;
			tok = reader->nextToken();
		} while (true);
	}
}
