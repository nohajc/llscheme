#include <cstdint>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "../include/ast.hpp"
#include "../include/environment.hpp"

namespace llscm {
	using namespace llvm;

	const int32_t ArgsAnyCount = -1;

	P_ScmObj makeScmList(vector<P_ScmObj> && elems) {
		P_ScmObj lst;
		P_ScmObj * lst_end = &lst;
		for (auto & e: elems) {
			*lst_end = make_unique<ScmCons>(move(e), nullptr);
			lst_end = &((ScmCons*)lst_end->get())->cdr;
		}
		*lst_end = make_unique<ScmNull>();
		return lst;
	}

	ostream &ScmInt::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << val << endl;
		return os;
	}

	ostream &ScmFloat::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << val << endl;
		return os;
	}

	ostream &ScmTrue::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "#t" << endl;
		return os;
	}

	ostream &ScmFalse::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "#f" << endl;
		return os;
	}

	ostream &ScmNull::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "null" << endl;
		return os;
	}

	ostream &ScmLit::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << val << endl;
		return os;
	}

	ostream &ScmCons::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "list:" << endl;
		const ScmCons * lst_end = this;

		while (lst_end) {
			lst_end->car->print(os, tabs + 1);
			//os << endl;
			lst_end = dynamic_cast<ScmCons*>(lst_end->cdr.get());
			// TODO: handle degenerate lists
		}

		return os;
	}

	ostream &ScmDefineVarSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "define [var]:" << endl;
		name->print(os, tabs + 1);
		//os << endl;
		val->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmDefineFuncSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "define [func]:" << endl;
		name->print(os, tabs + 1);
		//os << endl;
		arg_list->print(os, tabs + 1);
		//os << endl;
		body_list->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmLetSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "let:" << endl;
		bind_list->print(os, tabs + 1);
		//os << endl;
		body_list->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmLambdaSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "lambda:" << endl;
		arg_list->print(os, tabs + 1);
		//os << endl;
		body_list->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmQuoteSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "quote:" << endl;
		data->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmCall::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "call:" << endl;
		fexpr->print(os, tabs + 1);
		//os << endl;
		arg_list->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmIfSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "if:" << endl;
		cond_expr->print(os, tabs + 1);
		//os << endl;
		then_expr->print(os, tabs + 1);
		//os << endl;
		else_expr->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	P_ScmObj ScmSym::CT_Eval(P_ScmEnv env) {
		return P_ScmObj(this); // TODO: check whether the symbol is bound
	}

	P_ScmObj ScmCons::CT_Eval(P_ScmEnv env) {
		car = car->CT_Eval(env);
		cdr = cdr->CT_Eval(env);

		return P_ScmObj(this);
	}

	P_ScmObj ScmFunc::CT_Eval(P_ScmEnv env) {
		// TODO: bind arguments to a special type ScmArg,
		// create new function environment and eval body_list in it.
		// Argument values may not be known during compilation
		// but we need to know they are not unbound.

		return P_ScmObj(this);
	}

	P_ScmObj ScmCall::CT_Eval(P_ScmEnv env) {
		// TODO: decide if the call should be direct or indirect
		// fexpr can be either a symbol referencing ScmFunc
		// or it can be a call to function factory.
		// In the first case, the function pointer can be hardcoded,
		// in the second case we must emit an indirect call to the pointer
		// returned by the factory function. The pointer type will have
		// to be checked at runtime.
		fexpr = fexpr->CT_Eval(env);
		arg_list = arg_list->CT_Eval(env);
		return P_ScmObj(this);
	}

	P_ScmObj ScmDefineVarSyntax::CT_Eval(P_ScmEnv env) {
		val = val->CT_Eval(env);
		// Bind symbol to value
		env->set(*dynamic_cast<ScmSym*>(name.get()), val);

		return P_ScmObj(this);
	}

	P_ScmObj ScmDefineFuncSyntax::CT_Eval(P_ScmEnv env) {
		// TODO: convert to ScmDefineVarSyntax and ScmFunc.
		// Then call CT_Eval on the new object and return it.
		return P_ScmObj(this);
	}

	P_ScmObj ScmLambdaSyntax::CT_Eval(P_ScmEnv env) {
		// TODO: create new symbol (name) for the anonymous function.
		// Then convert this object to ScmDefineVarSyntax, prepend
		// the definition to env->prog and return the new symbol.
		return P_ScmObj(this);
	}

	P_ScmObj ScmIfSyntax::CT_Eval(P_ScmEnv env) {
		// TODO: detect dead branches
		cond_expr = cond_expr->CT_Eval(env);
		then_expr = then_expr->CT_Eval(env);
		else_expr = else_expr->CT_Eval(env);
		return P_ScmObj(this);
	}

	P_ScmObj ScmLetSyntax::CT_Eval(P_ScmEnv env) {
		// TODO: populate new environment according to bind_list,
		// then eval body_list in the new environment.
		return P_ScmObj(this);
	}
}

