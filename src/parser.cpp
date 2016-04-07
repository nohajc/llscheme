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
		cout << "Error: " << msg << endl;
		err_flag = true;
	}

	/*
	 * prog = form { form }
	 */
	ScmProg Parser::NT_Prog() {
		const Token * tok;
		ScmProg prog;
		bool first = true;

		D(cerr << "NT_Prog: " << endl);

		while (true) {
			tok = reader->nextToken();

			if (!tok) {
				if (first) {
					error("Program is empty.");
					return prog;
				}
				else break;
			}
			//D(cerr << tok->name << endl);
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
	 * expr = atom | "(" callsyn ")" | "'" data
	 */
	P_ScmObj Parser::NT_Form() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cerr << "NT_Form: " << endl);
		D(cerr << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();
			if (!tok) {
				error("Reached EOF while parsing a list.");
				return nullptr;
			}
			//D(cerr << tok->name << endl);
			if (tok->t == KWRD && tok->kw == KW_DEFINE) {
				// Current token is "define"
				reader->nextToken();
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

		if (tok->t == KWRD && tok->kw == KW_QUCHAR) {
			// Quote, short form
			reader->nextToken();
			obj = NT_Data();
			if (fail()) return nullptr;
			return make_unique<ScmQuoteSyntax>(move(obj));
		}

		// Anything other than "(" must be an atom
		return NT_Atom(false);
	}

	/*
	 * callsyn = "lambda" "(" symlist ")" body
	 *		   | "quote" data
	 *		   | "if" expr expr expr
	 *		   | "let" "(" bindlist ")" body
	 *		   | expr { expr }
	 */
	P_ScmObj Parser::NT_CallOrSyntax() {
		const Token * tok = reader->currToken();
		P_ScmObj expr, ce, te, ee;
		vector<P_ScmObj> lst;

		D(cerr << "NT_CallOrSyntax: " << endl);
		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

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
					tok = reader->nextToken();
					if (!tok || (tok->t == KWRD && tok->kw == KW_RPAR)) {
						error("Expected atom or list to quote.");
						return nullptr;
					}
					expr = NT_Data();
					if (fail()) return nullptr;
					reader->nextToken();
					return make_unique<ScmQuoteSyntax>(move(expr));
				case KW_IF:
					tok = reader->nextToken();
					if (!tok || (tok->t == KWRD && tok->kw == KW_RPAR)) {
						error("Missing condition expression.");
						return nullptr;
					}
					ce = NT_Expr();
					if (fail()) return nullptr;
					tok = reader->nextToken();
					if (!tok || (tok->t == KWRD && tok->kw == KW_RPAR)) {
						error("Missing then expression.");
						return nullptr;
					}
					te = NT_Expr();
					if (fail()) return nullptr;
					tok = reader->nextToken();
					if (!tok || (tok->t == KWRD && tok->kw == KW_RPAR)) {
						error("Missing else expression.");
						return nullptr;
					}
					ee = NT_Expr();
					if (fail()) return nullptr;
					reader->nextToken();
					return make_unique<ScmIfSyntax>(move(ce), move(te), move(ee));
				case KW_LET:
					D(cerr << "NT_Let: " << endl);

					if (!match(reader->nextToken(), Token(KW_LPAR))) {
						return nullptr;
					}
					reader->nextToken();
					//D(cerr << tok->name << endl);

					expr = NT_BindList();
					if (fail()) return nullptr;
					if (!match(reader->currToken(), Token(KW_RPAR))) {
						return nullptr;
					}
					reader->nextToken();
					//D(cerr << tok->name << endl);

					return make_unique<ScmLetSyntax>(move(expr), NT_Body());
				case KW_LPAR:
					break; // Fall through to function call
				case KW_RPAR:
					error("Missing function expression or syntax keyword.");
					return nullptr;
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
	 */
	P_ScmObj Parser::NT_Def() {
		const Token * tok = reader->currToken();
		P_ScmObj name, lst, expr;

		D(cerr << "NT_Def: " << endl);

		if (!tok) {
			error("Reached EOF while parsing a definition.");
			return nullptr;
		}
		D(cerr << tok->name << endl);

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
		//D(cerr << tok->name << endl);
		if (tok && tok->t == KWRD && tok->kw == KW_RPAR) {
			error("Missing expression in variable definition.");
			return nullptr;
		}

		expr = NT_Expr();
		if (fail()) return nullptr;
		reader->nextToken();
		//D(cerr << tok->name << endl);

		return make_unique<ScmDefineVarSyntax>(move(name), move(expr));
	}

	/*
	 * expr = atom | ( "(" callsyn ")" ) | "'" data
	 */
	P_ScmObj Parser::NT_Expr() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cerr << "NT_Expr: " << endl);

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
		D(cerr << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_QUCHAR) {
			// Quote, short form
			reader->nextToken();
			obj = NT_Data();
			if (fail()) return nullptr;
			return make_unique<ScmQuoteSyntax>(move(obj));
		}

		return NT_Atom(false);
	}

	/*
	 * data = atom | ( "(" list ")" ) | "'" data
	 */
	P_ScmObj Parser::NT_Data() {
		const Token * tok = reader->currToken();
		P_ScmObj obj;

		D(cerr << "NT_Data: " << endl);

		if (!tok) {
			error("Expected atom or list.");
			return nullptr;
		}

		if (tok->t == KWRD && tok->kw == KW_LPAR) {
			tok = reader->nextToken();

			if (!tok) {
				error("Reached EOF while parsing a list.");
				return nullptr;
			}

			D(cerr << tok->name << endl);

			if (tok->t == KWRD && tok->kw == KW_RPAR) {
				// Empty list
				D(cerr << "EMPTY LIST" << endl);
				obj = make_unique<ScmNull>(true);
			}
			else {
				obj = NT_List();
			}
			if (fail()) return nullptr;

			if (!match(reader->currToken(), Token(KW_RPAR))) {
				return nullptr;
			}
			return obj;
		}

		// Short form quote inside quote (eval as symbol "quote")
		if (tok->t == KWRD && tok->kw == KW_QUCHAR) {
			// Quote, short form
			reader->nextToken();
			if (fail()) return nullptr;
			return makeScmList({make_unique<ScmSym>("quote"), NT_Data()});
		}

		D(cerr << tok->name << endl);
		return NT_Atom(true);
	}

	/*
	 * atom = str | sym | int | float | true | false | null
	 */
	P_ScmObj Parser::NT_Atom(bool quoted) {
		const Token * tok = reader->currToken();

		D(cerr << "NT_Atom: " << endl);

		switch (tok->t) {
			case STR:
				return make_unique<ScmStr>(tok->name);
			case SYM:
				return make_unique<ScmSym>(tok->name);
			case INT:
				return make_unique<ScmInt>(tok->int_val);
			case FLOAT:
				return make_unique<ScmFloat>(tok->float_val);
			case ERR:
				err_flag = true;
				return nullptr;
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

		D(cerr << "NT_List: " << endl);
		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

		D(cerr << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// End of list
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

		D(cerr << "NT_BindList" << endl);
		if (!tok) {
			error("Reached EOF while parsing a list.");
			return nullptr;
		}

		D(cerr << tok->name << endl);

		if (tok->t == KWRD && tok->kw == KW_RPAR) {
			// Empty list
			return make_unique<ScmNull>();
		}
		if (!match(reader->currToken(), Token(KW_LPAR))) {
			return nullptr;
		}
		tok = reader->nextToken();
		//D(cerr << tok->name << endl);

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
		//D(cerr << tok->name << endl);

		vec.push_back(NT_Expr());
		if (!match(reader->nextToken(), Token(KW_RPAR))) {
			return nullptr;
		}
		reader->nextToken();
		//D(cerr << tok->name << endl);

		return make_unique<ScmCons>(makeScmList(move(vec)), NT_BindList());
	}

	/*
	 * body = { form } ; with at least one expr
	 */
	P_ScmObj Parser::NT_Body() {
		vector<P_ScmObj> lst;
		const Token * tok = reader->currToken();
		P_ScmObj obj;
		bool expr = false;

		D(cerr << "NT_Body" << endl);

		do {
			if (!tok) {
				error("Reached EOF while parsing a body.");
				return nullptr;
			}
			if (tok->t == KWRD && tok->kw == KW_RPAR) {
				if (!expr) {
					error("Missing expression at the end of a body.");
					return nullptr;
				}
				return makeScmList(move(lst));
			}
			obj = NT_Form();
			if (obj->t == T_DEF) {
				expr = false;
			}
			else {
				expr = true;
			}
			lst.push_back(move(obj));
			if (fail()) return nullptr;
			tok = reader->nextToken();
		} while (true);
	}
}
