#ifndef LLSCHEME_TYPES_HPP
#define LLSCHEME_TYPES_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <llvm/ADT/STLExtras.h>

namespace llscm {
	using namespace std;

	extern const int32_t ArgsAnyCount;

	enum ScmType {
		T_INT,
		T_FLOAT,
		T_STR,
		T_SYM,
		T_CONS,
		T_TRUE,
		T_FALSE,
		T_NULL,
		T_EXPR,
		T_FUNC,
		T_CALL,
		T_VECTOR
	};

	/*
	 * This class hierarchy is used to generate AST.
	 * It is created by parsing the source file.
	 * First we just read atoms and lists creating
	 * corresponding ScmCons cells with symbols, strings, numbers etc.
	 * Then we do the semantic analysis and compile-time evaluation:
	 * Evaluating defines, looking up functions by their names, creating nodes
	 * for lambdas, function calls, expressions and processing macros.
	 */
	class ScmObj {
	protected:
		void printTabs(ostream & os, int tabs) const {
			for (int i = 0; i < tabs; ++i) os << "\t";
		}
	public:
		ScmObj(ScmType type) {
			t = type;
		}

		virtual ~ScmObj() {}
		virtual ostream & print(ostream & os, int tabs = 0) const = 0;
		friend ostream & operator<<(ostream & os, const ScmObj & obj) {
			return obj.print(os);
		}

		ScmType t;
	};

	typedef unique_ptr<ScmObj> P_ScmObj;
	typedef shared_ptr<ScmObj> ShP_ScmObj;
	typedef char scm_op;

	class ScmInt: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << val;
			return os;
		}

	public:
		ScmInt(int64_t value): ScmObj(T_INT) {
			val = value;
		}

		int64_t val;
	};

	class ScmFloat: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << val;
			return os;
		}
	public:
		ScmFloat(double value): ScmObj(T_FLOAT) {
			val = value;
		}

		double val;
	};

	class ScmTrue: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "#t";
			return os;
		}
	public:
		ScmTrue(): ScmObj(T_TRUE) {}
	};

	class ScmFalse: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "#f";
			return os;
		}
	public:
		ScmFalse(): ScmObj(T_FALSE) {}
	};

	class ScmNull: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "null";
			return os;
		}
	public:
		ScmNull(): ScmObj(T_NULL) {}
	};

	class ScmLit: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << val;
			return os;
		}
	public:
		ScmLit(ScmType type, const string & value):
			ScmObj(type), val(value) {}

		int32_t length;
		string val;
	};

	class ScmStr: public ScmLit {
	public:
		ScmStr(const string & value): ScmLit(T_STR, value) {}
	};

	class ScmSym: public ScmLit {
	public:
		ScmSym(const string & value): ScmLit(T_SYM, value) {}
	};

	class ScmCons: public ScmObj {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "list:" << endl;
			const ScmCons * lst_end = this;

			while (lst_end) {
				lst_end->car->print(os, tabs + 1);
				os << endl;
				lst_end = dynamic_cast<ScmCons*>(lst_end->cdr.get());
				// TODO: handle degenerate lists
			}

			return os;
		}
	public:
		ScmCons(P_ScmObj pcar, P_ScmObj pcdr):
			ScmObj(T_CONS), car(move(pcar)), cdr(move(pcdr)) {}

		//virtual ~ScmCons();

		P_ScmObj car;
		P_ScmObj cdr;
	};

	class ScmExpr: public ScmObj {
	public:
		ScmExpr(): ScmObj(T_EXPR) {}
		//virtual ~ScmExpr();
	};

	// When args and bodies are set to nullptr, ScmFunc
	// is a reference to an external runtime library function.
	// There will be a compile-time environment populated with
	// native functions that will also be updated with user definitions.
	class ScmFunc: public ScmObj {
	public:
		ScmFunc(const string & fname, int32_t argc, P_ScmObj args, P_ScmObj bodies):
			ScmObj(T_FUNC), name(fname), arg_list(move(args)), body_list(move(bodies)) {
			argc_expected = argc;
		}
		//virtual ~ScmFunc();

		string name;
		int32_t argc_expected;
		P_ScmObj arg_list;
		P_ScmObj body_list;
		// scm_env def_env; // TODO
	};

	class ScmCall: public ScmObj {
	public:
		ScmCall(ShP_ScmObj pfunc, P_ScmObj args):
			ScmObj(T_CALL), func(pfunc), arg_list(move(args)) {}

		ShP_ScmObj func;
		P_ScmObj arg_list;
	};

	/* Classes derived from ScmInlineCall represent primitive functions
	 * such as arithmetic operations for which we want to emit instructions
	 * inline instead of an explicit function call.
	 */
	class ScmInlineCall: public ScmCall {
	public:
		ScmInlineCall(P_ScmObj args): ScmCall(nullptr, move(args)) {}
	};

	class ScmDefineSyntax: public ScmExpr {
	};

	class ScmDefineVarSyntax: public ScmDefineSyntax {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "define [var]:" << endl;
			name->print(os, tabs + 1);
			os << endl;
			val->print(os, tabs + 1);
			os << endl;
			return os;
		}
	public:
		ScmDefineVarSyntax(P_ScmObj n, P_ScmObj v):
			name(move(n)), val(move(v)) {}

		P_ScmObj name;
		P_ScmObj val;
	};

	class ScmDefineFuncSyntax: public ScmDefineSyntax {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "define [func]:" << endl;
			name->print(os, tabs + 1);
			os << endl;
			arg_list->print(os, tabs + 1);
			os << endl;
			body_list->print(os, tabs + 1);
			os << endl;
			return os;
		}
	public:
		ScmDefineFuncSyntax(P_ScmObj n, P_ScmObj al, P_ScmObj b):
			name(move(n)), arg_list(move(al)), body_list(move(b)) {}

		P_ScmObj name;
		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmLambdaSyntax: public ScmExpr {

	};

	class ScmQuoteSyntax: public ScmExpr {

	};

	class ScmIfSyntax: public ScmExpr {

	};

	class ScmLetSyntax: public ScmDefineSyntax {
		virtual ostream & print(ostream & os, int tabs = 0) const {
			printTabs(os, tabs);
			os << "let:" << endl;
			bind_list->print(os, tabs + 1);
			os << endl;
			body_list->print(os, tabs + 1);
			os << endl;
			return os;
		}
	public:
		ScmLetSyntax(P_ScmObj bl, P_ScmObj b):
			bind_list(move(bl)), body_list(move(b)) {}

		P_ScmObj bind_list;
		P_ScmObj body_list;
	};

	P_ScmObj makeScmList(vector<P_ScmObj> && elems);
}

#endif //LLSCHEME_TYPES_HPP