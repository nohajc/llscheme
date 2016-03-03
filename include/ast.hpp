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

	//typedef unique_ptr<ScmObj> P_ScmObj;
	typedef shared_ptr<ScmObj> P_ScmObj;
	typedef char scm_op;

	class ScmInt: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

	public:
		ScmInt(int64_t value): ScmObj(T_INT) {
			val = value;
		}

		int64_t val;
	};

	class ScmFloat: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

	public:
		ScmFloat(double value): ScmObj(T_FLOAT) {
			val = value;
		}

		double val;
	};

	class ScmTrue: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

	public:
		ScmTrue(): ScmObj(T_TRUE) {}
	};

	class ScmFalse: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

	public:
		ScmFalse(): ScmObj(T_FALSE) {}
	};

	class ScmNull: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

	public:
		ScmNull(): ScmObj(T_NULL) {}
	};

	class ScmLit: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

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
		bool operator==(const ScmSym & other) const {
			return val == other.val;
		}
	};

	class ScmCons: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;

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
		ScmFunc(int32_t argc, P_ScmObj args = nullptr, P_ScmObj bodies = nullptr):
				ScmObj(T_FUNC), arg_list(move(args)), body_list(move(bodies)) {
			argc_expected = argc;
		}
		//virtual ~ScmFunc();

		int32_t argc_expected;
		P_ScmObj arg_list;
		P_ScmObj body_list;
		// ScmEnv def_env; // TODO
	};

	class ScmConsFunc: public ScmFunc {
	public:
		ScmConsFunc(): ScmFunc(2) {}
	};

	class ScmCarFunc: public ScmFunc {
	public:
		ScmCarFunc(): ScmFunc(2) {}
	};

	class ScmCdrFunc: public ScmFunc {
	public:
		ScmCdrFunc(): ScmFunc(2) {}
	};

	class ScmPlusFunc: public ScmFunc {
	public:
		ScmPlusFunc(): ScmFunc(ArgsAnyCount) {}
	};

	class ScmMinusFunc: public ScmFunc {
	public:
		ScmMinusFunc(): ScmFunc(ArgsAnyCount) {}
	};

	class ScmCall: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;
	public:
		ScmCall(P_ScmObj f, P_ScmObj args): // Unresolved call
				ScmObj(T_CALL), fexpr(move(f)), arg_list(move(args)) {}

		P_ScmObj fexpr; // Expression returning a function object
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
		virtual ostream & print(ostream & os, int tabs) const;

	public:
		ScmDefineVarSyntax(P_ScmObj n, P_ScmObj v):
				name(move(n)), val(move(v)) {}

		P_ScmObj name;
		P_ScmObj val;
	};

	class ScmDefineFuncSyntax: public ScmDefineSyntax {
		virtual ostream & print(ostream & os, int tabs) const;
	public:
		ScmDefineFuncSyntax(P_ScmObj n, P_ScmObj al, P_ScmObj b):
				name(move(n)), arg_list(move(al)), body_list(move(b)) {}

		P_ScmObj name;
		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmLambdaSyntax: public ScmExpr {
		virtual ostream & print(ostream & os, int tabs) const;
	public:
		ScmLambdaSyntax(P_ScmObj al, P_ScmObj b):
				arg_list(move(al)), body_list(move(b)) {}

		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmQuoteSyntax: public ScmExpr {
		virtual ostream & print(ostream & os, int tabs) const;
	public:
		ScmQuoteSyntax(P_ScmObj d): data(move(d)) {}

		P_ScmObj data;
	};

	class ScmIfSyntax: public ScmExpr {
		virtual ostream & print(ostream & os, int tabs) const;
	public:
		ScmIfSyntax(P_ScmObj ce, P_ScmObj te, P_ScmObj ee):
				cond_expr(move(ce)), then_expr(move(te)), else_expr(move(ee)) {}

		P_ScmObj cond_expr;
		P_ScmObj then_expr;
		P_ScmObj else_expr;
	};

	class ScmLetSyntax: public ScmExpr {
		virtual ostream & print(ostream & os, int tabs) const;
	public:
		ScmLetSyntax(P_ScmObj bl, P_ScmObj b):
				bind_list(move(bl)), body_list(move(b)) {}

		P_ScmObj bind_list;
		P_ScmObj body_list;
	};

	P_ScmObj makeScmList(vector<P_ScmObj> && elems);
}

// Define a specialization of std::hash<T> for ScmSym class
namespace std {
	template <>
	class hash<llscm::ScmSym> {
	public:
		size_t operator()(const llscm::ScmSym & sym) const {
			return hash<string>()(sym.val);
		}
	};
}

#endif //LLSCHEME_TYPES_HPP