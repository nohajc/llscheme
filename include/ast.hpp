#ifndef LLSCHEME_TYPES_HPP
#define LLSCHEME_TYPES_HPP

#include <cstdint>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <llvm/ADT/STLExtras.h>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "common.hpp"
#include "ast_visitor.hpp"

namespace llscm {
	using namespace std;
	using namespace llvm;

	extern const int32_t ArgsAnyCount;

	enum ScmType {
		T_INT,
		T_FLOAT,
		T_STR,
		T_SYM,
		T_ARG,
		T_REF,
		T_CONS,
		T_TRUE,
		T_FALSE,
		T_NULL,
		T_DEF,
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
	class ScmObj;
	class ScmEnv;

	typedef shared_ptr<ScmObj> P_ScmObj;
	typedef shared_ptr<ScmEnv> P_ScmEnv;

	class ScmProg: public list<P_ScmObj>, public Visitable<ScmProg> {};

	class ScmObj: public enable_shared_from_this<ScmObj>, public Visitable<ScmObj> {
	protected:
		void printTabs(ostream & os, int tabs) const {
			for (int i = 0; i < tabs; ++i) os << "\t";
		}
	public:
		ScmObj(ScmType type) {
			t = type;
		}

		virtual ostream & print(ostream & os, int tabs = 0) const {
			return os;
		}
		virtual ostream & printSrc(ostream & os) const {
			return os;
		}
		friend ostream & operator<<(ostream & os, const ScmObj & obj) {
			return obj.print(os);
		}

		virtual P_ScmObj CT_Eval(P_ScmEnv env) {
			return shared_from_this();
		}

		ScmType t;
		Value * IR_val;
	};

	class ScmArg: public ScmObj {
	public:
		ScmArg(): ScmObj(T_ARG) {}
	};

	typedef shared_ptr<ScmObj> P_ScmObj;

	class ScmInt: public ScmObj, public Visitable<ScmInt> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;

	public:
		ScmInt(int64_t value): ScmObj(T_INT) {
			val = value;
		}

		int64_t val;
	};

	class ScmFloat: public ScmObj, public Visitable<ScmFloat> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmFloat(double value): ScmObj(T_FLOAT) {
			val = value;
		}

		double val;
	};

	class ScmTrue: public ScmObj, public Visitable<ScmTrue> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmTrue(): ScmObj(T_TRUE) {}
	};

	class ScmFalse: public ScmObj, public Visitable<ScmFalse> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmFalse(): ScmObj(T_FALSE) {}
	};

	class ScmNull: public ScmObj, public Visitable<ScmNull> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmNull(): ScmObj(T_NULL) {}
	};

	class ScmLit: public ScmObj {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmLit(ScmType type, const string & value):
				ScmObj(type), val(value) {}

		string val;
	};

	class ScmStr: public ScmLit, public Visitable<ScmStr> {
	public:
		ScmStr(const string & value): ScmLit(T_STR, value) {}
	};

	class ScmSym: public ScmLit, public Visitable<ScmSym> {
	public:
		ScmSym(const string & value): ScmLit(T_SYM, value) {}
		bool operator==(const ScmSym & other) const {
			return val == other.val;
		}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);
	};

	// ScmRef is a resolved symbol.
	// As we traverse the code and change symbol bindings dynamically
	// (redefinitions are permitted), we need to establish a permanent binding
	// for each occurence of every symbol. So, in the CT_Eval phase, we replace
	// every ScmSym with ScmRef (except quoted symbols).
	class ScmRef: public ScmLit, public Visitable<ScmRef> {
		// TODO: we need multiple reference types: refs to globals, locals and closure data,
		// maybe even stack locals, heap locals and heap closure data (with the level
		// of indirection specified - because there can be closures inside of closures).
		// Each function would then have its own pointer to heap locals
		// which could be passed to closure function as an implicit hidden argument.
	public:
		ScmRef(const string & name, P_ScmObj obj):
				ScmLit(T_REF, name), ref_obj(obj) {}
		P_ScmObj ref_obj;
	};

	class ScmCons: public ScmObj, public Visitable<ScmCons> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
		ssize_t len;
	public:
		ScmCons(P_ScmObj pcar, P_ScmObj pcdr):
				ScmObj(T_CONS), car(move(pcar)), cdr(move(pcdr)) {
			len = -1;
		}
		virtual ostream & printElems(ostream & os) const;

		virtual P_ScmObj CT_Eval(P_ScmEnv env);
		ssize_t length() {
			if (len >= 0) return len;
			ssize_t cnt = 0;
			each([&cnt](P_ScmObj e) { cnt++; });
			return len = cnt;
		}

		template<typename F>
		void each(F && lambda) {
			ScmObj * obj = this;
			while (obj->t != T_NULL) {
				assert(obj->t == T_CONS);
				ScmCons * lst = (ScmCons*)obj;
				lambda(lst->car);
				obj = lst->cdr.get();
			}
		}

		P_ScmObj car;
		P_ScmObj cdr;
	};

	class ScmExpr: public ScmObj {
	public:
		ScmExpr(): ScmObj(T_EXPR) {}
	};

	// When args and bodies are set to nullptr, ScmFunc
	// is a reference to an external runtime library function.
	// There will be a compile-time environment populated with
	// native functions that will also be updated with user definitions.
	class ScmFunc: public ScmObj, public Visitable<ScmFunc> {
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmFunc(int32_t argc, P_ScmObj args = nullptr, P_ScmObj bodies = nullptr):
				ScmObj(T_FUNC), arg_list(move(args)), body_list(move(bodies)) {
			argc_expected = argc;
		}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		int32_t argc_expected;
		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmConsFunc: public ScmFunc, public Visitable<ScmConsFunc> {
	public:
		ScmConsFunc(): ScmFunc(2) {}
	};

	class ScmCarFunc: public ScmFunc, public Visitable<ScmCarFunc> {
	public:
		ScmCarFunc(): ScmFunc(1) {}
	};

	class ScmCdrFunc: public ScmFunc, public Visitable<ScmCdrFunc> {
	public:
		ScmCdrFunc(): ScmFunc(1) {}
	};

	class ScmNullFunc: public ScmFunc, public Visitable<ScmNullFunc> {
	public:
		ScmNullFunc(): ScmFunc(1) {}
	};

	class ScmPlusFunc: public ScmFunc, public Visitable<ScmPlusFunc> {
	public:
		ScmPlusFunc(): ScmFunc(ArgsAnyCount) {}
	};

	class ScmMinusFunc: public ScmFunc, public Visitable<ScmMinusFunc> {
	public:
		ScmMinusFunc(): ScmFunc(ArgsAnyCount) {}
	};

	class ScmTimesFunc: public ScmFunc, public Visitable<ScmTimesFunc> {
	public:
		ScmTimesFunc(): ScmFunc(ArgsAnyCount) {}
	};

	class ScmDivFunc: public ScmFunc, public Visitable<ScmDivFunc> {
	public:
		ScmDivFunc(): ScmFunc(ArgsAnyCount) {}
	};

	class ScmGtFunc: public ScmFunc, public Visitable<ScmGtFunc> {
	public:
		ScmGtFunc(): ScmFunc(2) {}
	};

	class ScmPrintFunc: public ScmFunc, public Visitable<ScmPrintFunc> {
	public:
		ScmPrintFunc(): ScmFunc(1) {}
	};

	class ScmCall: public ScmObj, public Visitable<ScmCall> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmCall(P_ScmObj f, P_ScmObj args): // Unresolved call
				ScmObj(T_CALL), fexpr(move(f)), arg_list(move(args)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj fexpr; // Expression returning a function object
		P_ScmObj arg_list;
		bool indirect;
	};

	/* Classes derived from ScmInlineCall represent primitive functions
	 * such as arithmetic operations for which we want to emit instructions
	 * inline instead of an explicit function call.
	 */
	class ScmInlineCall: public ScmCall, public Visitable<ScmInlineCall> {
	public:
		ScmInlineCall(P_ScmObj args): ScmCall(nullptr, move(args)) {}
	};

	class ScmDefineSyntax: public ScmObj, public Visitable<ScmDefineSyntax> {
	public:
		ScmDefineSyntax(): ScmObj(T_DEF) {}
	};

	class ScmDefineVarSyntax: public ScmDefineSyntax, public Visitable<ScmDefineVarSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmDefineVarSyntax(P_ScmObj n, P_ScmObj v):
				name(move(n)), val(move(v)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj name;
		P_ScmObj val;
	};

	class ScmDefineFuncSyntax: public ScmDefineSyntax, public Visitable<ScmDefineFuncSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmDefineFuncSyntax(P_ScmObj n, P_ScmObj al, P_ScmObj b):
				name(move(n)), arg_list(move(al)), body_list(move(b)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj name;
		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmLambdaSyntax: public ScmExpr, public Visitable<ScmLambdaSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmLambdaSyntax(P_ScmObj al, P_ScmObj b):
				arg_list(move(al)), body_list(move(b)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmQuoteSyntax: public ScmExpr, public Visitable<ScmQuoteSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmQuoteSyntax(P_ScmObj d): data(move(d)) {}

		P_ScmObj data;
	};

	class ScmIfSyntax: public ScmExpr, public Visitable<ScmIfSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmIfSyntax(P_ScmObj ce, P_ScmObj te, P_ScmObj ee):
				cond_expr(move(ce)), then_expr(move(te)), else_expr(move(ee)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj cond_expr;
		P_ScmObj then_expr;
		P_ScmObj else_expr;
	};

	class ScmLetSyntax: public ScmExpr, public Visitable<ScmLetSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmLetSyntax(P_ScmObj bl, P_ScmObj b):
				bind_list(move(bl)), body_list(move(b)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

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