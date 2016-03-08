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
		P_ScmObj sym = shared_ptr<ScmObj>(this);
		P_ScmObj last_sym;

		do {
			last_sym = sym;
			sym = env->get(last_sym);
		} while(sym && sym->t == T_SYM);

		if (!sym) {
			env->error(DPC<ScmSym>(last_sym)->val + " is not defined.");
			return nullptr;
		}

		return last_sym;
	}

	P_ScmObj ScmCons::CT_Eval(P_ScmEnv env) {
		car = car->CT_Eval(env);
		cdr = cdr->CT_Eval(env);

		return P_ScmObj(this);
	}

	P_ScmObj ScmFunc::CT_Eval(P_ScmEnv env) {
		if (arg_list && body_list) {
			// Create a new environment for the function
			// and save it in the object.
			// We will work with it during code generation.
			// ScmEnv must hold a reference to its corresponding function
			// because when accessing variables from closures, we need to know where they are defined.
			P_ScmEnv new_env = make_shared<ScmEnv>(env->prog, env);
			new_env->context = P_ScmObj(this);

			// Bind all argument names to ScmArg - we need to tell them apart from unbound variables.
			P_ScmObj arg_type = make_shared<ScmArg>();

			assert(arg_list->t == T_CONS);
			DPC<ScmCons>(arg_list)->each([&new_env, &arg_type](P_ScmObj e) {
				new_env->set(e, arg_type);
			});

			// Eval function bodies in the function environment
			assert(body_list->t == T_CONS);
			DPC<ScmCons>(body_list)->each([&new_env](P_ScmObj & e) {
				e = e->CT_Eval(new_env);
			});

			fn_env = new_env;
		}
		return P_ScmObj(this);
	}

	P_ScmObj ScmCall::CT_Eval(P_ScmEnv env) {
		P_ScmObj func;
		fexpr = fexpr->CT_Eval(env);
		// After evaluation of fexpr, it can either be ScmSym bound to ScmFunc
		// or some ScmExpr (Quote, If, Let). It cannot be Lambda because that is reduced
		// to the first case after fexpr->CT_Eval. We know for certain that Quote won't
		// be a valid case as it always returns data.
		if (fexpr->t == T_SYM && (func = env->get(fexpr))->t == T_FUNC) {
			// Function is known at compilation time - we can hardcode its pointer
			indirect = false;
			fexpr = func; // We don't call CT_Eval on func here - that's done from its definition.
		}
		else if (fexpr->t == T_EXPR && !DPC<ScmQuoteSyntax>(fexpr)) {
			// TODO: this could be optimized further...
			// Maybe we could traverse the expression recursively and determine
			// whether it returns ScmFunc. That way we could eliminate more invalid
			// expressions at compilation time.
			// We only get the particular function pointer at runtime.
			// Therefore we will have to verify it is in fact pointer to a function.
			indirect = true;
		}
		else {
			env->error("Invalid expression in the function slot.");
			return nullptr;
		}

		arg_list = arg_list->CT_Eval(env);
		return P_ScmObj(this);
	}

	P_ScmObj ScmDefineVarSyntax::CT_Eval(P_ScmEnv env) {
		val = val->CT_Eval(env);
		// Bind symbol to value
		env->set(name, val);

		return P_ScmObj(this);
	}

	P_ScmObj ScmDefineFuncSyntax::CT_Eval(P_ScmEnv env) {
		// Converts to ScmDefineVarSyntax with ScmFunc.
		// Then calls CT_Eval on the new object and return it.

		P_ScmObj func = make_shared<ScmFunc>(
				DPC<ScmCons>(arg_list)->length(),
				move(arg_list), move(body_list)
		);
		P_ScmObj def_var = make_shared<ScmDefineVarSyntax>(name, func);

		return def_var->CT_Eval(env);
	}

	P_ScmObj ScmLambdaSyntax::CT_Eval(P_ScmEnv env) {
		// Creates a new symbol (name) for the anonymous function.
		string fname = env->getUniqID("lambda");
		P_ScmObj fsym = make_shared<ScmSym>(fname);

		// Converts this object to ScmDefineVarSyntax.
		P_ScmObj func = make_shared<ScmFunc>(
				DPC<ScmCons>(arg_list)->length(),
				move(arg_list), move(body_list)
		);
		P_ScmObj def_var = make_shared<ScmDefineVarSyntax>(fsym, func);
		// Prepends the definition to env->prog, runs CT_Eval on the definition.
		env->prog.push_front(def_var->CT_Eval(env));

		// We've moved the inplace lambda definition to a separate node
		// at the start of ScmProg and now we just return the unique symbol bound to it.
		return fsym;
	}

	P_ScmObj ScmIfSyntax::CT_Eval(P_ScmEnv env) {
		// TODO: detect dead branches
		cond_expr = cond_expr->CT_Eval(env);
		then_expr = then_expr->CT_Eval(env);
		else_expr = else_expr->CT_Eval(env);
		return P_ScmObj(this);
	}

	P_ScmObj ScmLetSyntax::CT_Eval(P_ScmEnv env) {
		P_ScmEnv let_env = make_shared<ScmEnv>(env->prog, env);

		assert(bind_list->t == T_CONS);
		// Populates new environment according to bind_list.
		DPC<ScmCons>(bind_list)->each([&let_env, &env](P_ScmObj e) {
			assert(e->t == T_CONS);
			shared_ptr<ScmCons> kv = DPC<ScmCons>(e);
			P_ScmObj id = kv->car;

			assert(kv->cdr->t == T_CONS);
			P_ScmObj & expr = DPC<ScmCons>(kv->cdr)->car;
			expr = expr->CT_Eval(env);
			let_env->set(id, expr);
		});
		// Evals body_list in the new environment.
		body_list = body_list->CT_Eval(let_env);

		return P_ScmObj(this);
	}
}

