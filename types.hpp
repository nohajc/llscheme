#include <cstdint>
#include <memory>
#include <string>
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
	public:
		ScmObj(ScmType type) {
			t = type;
		}

		virtual ~ScmObj() {}

		ScmType t;
	};

	typedef unique_ptr<ScmObj> P_ScmObj;
	typedef shared_ptr<ScmObj> ShP_ScmObj;
	typedef char scm_op;

	class ScmInt: public ScmObj {
	public:
		ScmInt(int64_t value): ScmObj(T_INT) {
			val = value;
		}

		int64_t val;
	};

	class ScmFloat: public ScmObj {
	public:
		ScmFloat(double value): ScmObj(T_FLOAT) {
			val = value;
		}

		double val;
	};

	class ScmTrue: public ScmObj {
	public:
		ScmTrue(): ScmObj(T_TRUE) {}
	};

	class ScmFalse: public ScmObj {
	public:
		ScmFalse(): ScmObj(T_FALSE) {}
	};

	class ScmNull: public ScmObj {
	public:
		ScmNull(): ScmObj(T_NULL) {}
	};

	class ScmLit: public ScmObj {
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

	};

	class ScmDefineSyntax: public ScmExpr {

	};

	class ScmLambdaSyntax: public ScmExpr {

	};

	class ScmQuoteSyntax: public ScmExpr {

	};

	class ScmIfSyntax: public ScmExpr {

	};

	class ScmLetSyntax: public ScmExpr {

	};
}
