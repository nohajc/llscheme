#ifndef LLSCHEME_AST_HPP
#define LLSCHEME_AST_HPP

#include <cstdint>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
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
		T_VECTOR,
		T_PROG
	};

	enum ScmLocType {
		T_GLOB,
		T_HEAP_LOC,
		T_STACK_LOC
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

	// TODO: handle forward references!
	// TODO: constant folding

	// TODO: introduce special syntax construct "require"
	// which will import functions from other compiled scheme modules.
	class ScmObj: public enable_shared_from_this<ScmObj>, public Visitable<ScmObj> {
	protected:
		void printTabs(ostream & os, int tabs) const {
			for (int i = 0; i < tabs; ++i) os << "\t";
		}
	public:
		ScmObj(ScmType type) {
			t = type;
			IR_val = nullptr;
			is_extern = false;
			location = T_STACK_LOC;
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

		// Location type specifier.
		// Possible types are: global, stack local, heap local.
		// ScmRef code generation depends on the type.
		ScmLocType location;

		bool is_extern;
		string exported_name;

		// Pointer to ScmFunc. We need it for heap locals
		// in the codegen phase so that we can look up their index in the right
		// table and using that, generate proper code for object access.
		ScmFunc * defined_in_func;
	};

	class ScmProg: public Visitable<ScmProg, ScmObj> {
		list<P_ScmObj> form_lst;
	public:
		ScmProg(): Visitable(T_PROG) {}
		/*void push_front(P_ScmObj o) {
			form_lst.push_front(o);
		}*/
		list<P_ScmObj>::iterator insert(list<P_ScmObj>::iterator pos, const P_ScmObj & o) {
			return form_lst.insert(pos, o);
		}
		void push_back(P_ScmObj o) {
			form_lst.push_back(o);
		}
		list<P_ScmObj>::iterator begin() {
			return form_lst.begin();
		}
		list<P_ScmObj>::iterator end() {
			return form_lst.end();
		}

		virtual P_ScmObj CT_Eval(P_ScmEnv env);
	};

	class ScmArg: public Visitable<ScmArg, ScmObj> {
	public:
		ScmArg(): Visitable(T_ARG) {}
	};

	typedef shared_ptr<ScmObj> P_ScmObj;

	class ScmInt: public Visitable<ScmInt, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;

	public:
		ScmInt(int64_t value): Visitable(T_INT) {
			val = value;
		}

		int64_t val;
	};

	class ScmFloat: public Visitable<ScmFloat, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmFloat(double value): Visitable(T_FLOAT) {
			val = value;
		}

		double val;
	};

	class ScmTrue: public Visitable<ScmTrue, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmTrue(): Visitable(T_TRUE) {}
	};

	class ScmFalse: public Visitable<ScmFalse, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmFalse(): Visitable(T_FALSE) {}
	};

	class ScmNull: public Visitable<ScmNull, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
		bool empty_list;
	public:
		ScmNull(bool elst = false): Visitable(T_NULL) {
			empty_list = elst;
		}
	};

	class ScmLit: public Visitable<ScmLit, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmLit(ScmType type, const string & value):
				Visitable(type), val(value) {}

		string val;
	};

	class ScmStr: public Visitable<ScmStr, ScmLit> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmStr(const string & value): Visitable(T_STR, value) {}
	};

	class ScmSym: public Visitable<ScmSym, ScmLit> {
	public:
		ScmSym(const string & value): Visitable(T_SYM, value) {}
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
	class ScmRef: public Visitable<ScmRef, ScmLit> {
		// There can be refs to globals, locals and closure data,
		// maybe even stack locals, heap locals and heap closure data (with the level
		// of indirection specified - because there can be closures inside of closures).
		// Each function would then have its own pointer to heap locals
		// which could be passed to closure function as an implicit hidden argument.
		weak_ptr<ScmObj> ref_obj_weak;
		P_ScmObj ref_obj;
		bool is_weak;
	public:
		// Store levels of indirection needed for accessing the ref_obj
		// obtained from ScmEnv::get. Non-zero value used for closure data,
		// not used for locals or globals.
		int num_of_levels_up;

		ScmRef(const string & name, P_ScmObj obj, int levels = 0, bool weak = true):
				Visitable(T_REF, name), ref_obj_weak(obj), num_of_levels_up(levels), is_weak(weak) {
			if (!is_weak) {
				ref_obj = obj;
			}
		}
		P_ScmObj refObj() {
			return ref_obj_weak.lock();
		}
	};

	class ScmCons: public Visitable<ScmCons, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
		int32_t len;
	public:
		ScmCons(P_ScmObj pcar, P_ScmObj pcdr):
				Visitable(T_CONS), car(move(pcar)), cdr(move(pcdr)) {
			len = -1;
		}
		virtual ostream & printElems(ostream & os) const;

		virtual P_ScmObj CT_Eval(P_ScmEnv env);
		int32_t length() {
			if (len >= 0) return len;
			int32_t cnt = 0;
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

	class ScmExpr: public Visitable<ScmExpr, ScmObj> {
	public:
		ScmExpr(): Visitable(T_EXPR) {}
	};

	// When args and bodies are set to nullptr, ScmFunc
	// is a reference to an external runtime library function.
	// There will be a compile-time environment populated with
	// native functions that will also be updated with user definitions.
	class ScmFunc: public Visitable<ScmFunc, ScmObj> {
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmFunc(int32_t argc, const string & fname, P_ScmObj args = nullptr, P_ScmObj bodies = nullptr):
				Visitable(T_FUNC), name(fname), arg_list(move(args)), body_list(move(bodies)) {
			argc_expected = argc;
			has_closure = false;
			passing_closure = false;
			IR_heap_storage = nullptr;
			IR_context_ptr = nullptr;
			IR_wrapper_fn_ptr = nullptr;
		}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);
		void addHeapLocal(P_ScmObj obj) {
			// Zero index is reserved for a pointer to the parent heap data
			heap_local_idx[obj.get()] = (int)heap_local_idx.size() + 1;
		}

		string name;
		int32_t argc_expected;
		P_ScmObj arg_list;
		P_ScmObj body_list;
		// Store map of heap locals' indices (or vector of locals)
		// that gets updated in the CT_Eval phase everytime we find
		// a reference to captured variable.
		unordered_map<ScmObj*, int32_t> heap_local_idx;
		// Functions with closure have a different calling convention:
		// First hidden argument is the context pointer.
		bool has_closure;
		// Function which takes context pointer and has to make it
		// available to another closure function defined inside it.
		bool passing_closure;
		Value * IR_heap_storage;
		Value * IR_context_ptr;
		Function * IR_wrapper_fn_ptr;
	};

	// Derived function classes for specialized code generation
	// of the function's bodies. We need this to support inlining.
	// TODO: inlining
	class ScmConsFunc: public Visitable<ScmConsFunc, ScmFunc> {
	public:
		ScmConsFunc();
	};

	class ScmCarFunc: public Visitable<ScmCarFunc, ScmFunc> {
	public:
		ScmCarFunc();
	};

	class ScmCdrFunc: public Visitable<ScmCdrFunc, ScmFunc> {
	public:
		ScmCdrFunc();
	};

	class ScmNullFunc: public Visitable<ScmNullFunc, ScmFunc> {
	public:
		ScmNullFunc();
	};

	class ScmPlusFunc: public Visitable<ScmPlusFunc, ScmFunc> {
	public:
		ScmPlusFunc();
	};

	class ScmMinusFunc: public Visitable<ScmMinusFunc, ScmFunc> {
	public:
		ScmMinusFunc();
	};

	class ScmTimesFunc: public Visitable<ScmTimesFunc, ScmFunc> {
	public:
		ScmTimesFunc();
	};

	class ScmDivFunc: public Visitable<ScmDivFunc, ScmFunc> {
	public:
		ScmDivFunc();
	};

	class ScmGtFunc: public Visitable<ScmGtFunc, ScmFunc> {
	public:
		ScmGtFunc();
	};

	class ScmNumEqFunc: public Visitable<ScmNumEqFunc, ScmFunc> {
	public:
		ScmNumEqFunc();
	};

	class ScmDisplayFunc: public Visitable<ScmDisplayFunc, ScmFunc> {
	public:
		ScmDisplayFunc();
	};

	class ScmCmdArgsFunc: public Visitable<ScmCmdArgsFunc, ScmFunc> {
	public:
		ScmCmdArgsFunc();
	};

	class ScmVecLenFunc: public Visitable<ScmVecLenFunc, ScmFunc> {
	public:
		ScmVecLenFunc();
	};

	class ScmVecRefFunc: public Visitable<ScmVecRefFunc, ScmFunc> {
	public:
		ScmVecRefFunc();
	};

	class ScmApplyFunc: public Visitable<ScmApplyFunc, ScmFunc> {
	public:
		ScmApplyFunc();
	};

	class ScmLengthFunc: public Visitable<ScmLengthFunc, ScmFunc> {
	public:
		ScmLengthFunc();
	};

	class ScmCall: public Visitable<ScmCall, ScmObj> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmCall(P_ScmObj f, P_ScmObj args): // Unresolved call
				Visitable(T_CALL), fexpr(move(f)), arg_list(move(args)) {
			argc = -1;
		}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj fexpr; // Expression returning a function object
		P_ScmObj arg_list;
		int32_t argc;
		bool indirect;
	};

	/*
	 * TODO: Delete this class. Instead we will have inlinable ScmFunc types.
	 * Each derived ScmFunc class will be able to generate inline code
	 * and the corresponding ScmCall will control whether it wants
	 * the inline or the full version of the function code.
	 */
	class ScmInlineCall: public Visitable<ScmInlineCall, ScmCall> {
	public:
		ScmInlineCall(P_ScmObj args): Visitable(nullptr, move(args)) {}
	};

	class ScmDefineSyntax: public Visitable<ScmDefineSyntax, ScmObj> {
	public:
		ScmDefineSyntax(): Visitable(T_DEF) {}
	};

	class ScmDefineVarSyntax: public Visitable<ScmDefineVarSyntax, ScmDefineSyntax> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmDefineVarSyntax(P_ScmObj n, P_ScmObj v):
				name(move(n)), val(move(v)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj name;
		P_ScmObj val;
	};

	class ScmDefineFuncSyntax: public Visitable<ScmDefineFuncSyntax, ScmDefineSyntax> {
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

	class ScmLambdaSyntax: public Visitable<ScmLambdaSyntax, ScmExpr> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmLambdaSyntax(P_ScmObj al, P_ScmObj b):
				arg_list(move(al)), body_list(move(b)) {}
		virtual P_ScmObj CT_Eval(P_ScmEnv env);

		P_ScmObj arg_list;
		P_ScmObj body_list;
	};

	class ScmQuoteSyntax: public Visitable<ScmQuoteSyntax, ScmExpr> {
		virtual ostream & print(ostream & os, int tabs) const;
		virtual ostream & printSrc(ostream & os) const;
	public:
		ScmQuoteSyntax(P_ScmObj d): data(move(d)) {}

		P_ScmObj data;
	};

	class ScmIfSyntax: public Visitable<ScmIfSyntax, ScmExpr> {
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

	class ScmLetSyntax: public Visitable<ScmLetSyntax, ScmExpr> {
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

#endif //LLSCHEME_AST_HPP