#include <cstdint>

#include <memory>
#include <algorithm>
#include <llvm/ADT/STLExtras.h>
#include <fs_helpers.hpp>
#include <lib_reader.hpp>
#include "../include/ast.hpp"
#include "../include/environment.hpp"
#include "../include/codegen.hpp"

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

	ostream &ScmInt::printSrc(ostream &os) const {
		os << val;
		return os;
	}


	ostream &ScmFloat::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << val << endl;
		return os;
	}

	ostream &ScmFloat::printSrc(ostream &os) const {
		os << val;
		return os;
	}


	ostream &ScmTrue::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "#t" << endl;
		return os;
	}

	ostream &ScmTrue::printSrc(ostream &os) const {
		os << "#t";
		return os;
	}


	ostream &ScmFalse::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "#f" << endl;
		return os;
	}

	ostream &ScmFalse::printSrc(ostream &os) const {
		os << "#f";
		return os;
	}


	ostream &ScmNull::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "null" << endl;
		return os;
	}

	ostream &ScmNull::printSrc(ostream &os) const {
		if (empty_list) {
			os << "()";
		}
		else {
			os << "null";
		}
		return os;
	}


	ostream &ScmLit::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << val << endl;
		return os;
	}

	ostream &ScmLit::printSrc(ostream & os) const {
		os << val;
		return os;
	}

	ostream &ScmStr::printSrc(ostream & os) const {
		os << '\"' << val << '\"';
		return os;
	}

	ostream &ScmStr::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << '\"' << val << '\"' << endl;
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

	ostream &ScmAndSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "and:" << endl;
		expr_list->print(os, tabs + 1);
		return os;
	}

	ostream &ScmOrSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "or:" << endl;
		expr_list->print(os, tabs + 1);
		return os;
	}

	ostream &ScmQuoteSyntax::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "quote:" << endl;
		data->print(os, tabs + 1);
		//os << endl;
		return os;
	}

	ostream &ScmQuoteSyntax::printSrc(ostream &os) const {
		os << "(quote ";
		data->printSrc(os);
		os << ")";
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

	P_ScmObj ScmProg::CT_Eval(P_ScmEnv env) {
		for (auto & e: form_lst) {
			e = e->CT_Eval(env);
			if (env->fail()) {
				return nullptr;
			}
		}

		env->checkUnRefs();
		if (env->fail()) {
			return nullptr;
		}
		// We don't really use the return value
		// but nullptr is error so we must return something.
		return *form_lst.rbegin();
	}


	P_ScmObj ScmSym::CT_Eval(P_ScmEnv env) {
		//P_ScmObj sym = shared_from_this();
		P_ScmObj dummy;
		P_ScmObj sym = shared_ptr<ScmObj>(dummy, this);
		P_ScmObj last_sym;
		string last_sym_name;
		int num_of_levels_up = 0;
		ScmFunc * def_func = nullptr;
		ScmLoc location;

		do {
			location = nullptr;
			//cout << "DEBUG: " << DPC<ScmSym>(sym)->val << endl;
			last_sym = sym;
			sym = env->get(last_sym, &location);
			if (num_of_levels_up != ScmEnv::GlobalLevel) {
				if (location->first == ScmEnv::GlobalLevel) {
					num_of_levels_up = location->first;
				}
				else {
					num_of_levels_up += location->first;
				}
			}
			def_func = location->second;
		} while(sym && sym->t == T_SYM);

		last_sym_name = DPC<ScmSym>(last_sym)->val;

		while (sym && sym->t == T_REF) {
			sym = DPC<ScmRef>(sym)->refObj();
		}
		//D(cerr << "BOUND TO: " << sym->t << endl);
		//D(cerr << "LEVELS: " << num_of_levels_up << endl);

		if (!sym) {
			//env->error(last_sym_name + " is not defined.");
			//return nullptr;
			shared_ptr<ScmRef> ref = make_shared<ScmRef>(last_sym_name, nullptr);
			env->getUnRefs().emplace(
				make_pair(
					*DPC<ScmSym>(last_sym), ScmEnv::CapturedRef{env, ref}
				)
			);
			return ref;
		}

		// Set type of sym (global, stack local or heap local),
		// set defining func of the sym and add sym to the
		// def_funcs's list of heap locals if necessary.
		if (num_of_levels_up == ScmEnv::GlobalLevel) {
			sym->location = T_GLOB;
		}
		else if (num_of_levels_up > 0) {
			D(cerr << "Closure data ref: ");
			D(cerr << sym.get());
			D(cerr << ", number of levels: " << num_of_levels_up << endl);
			sym->location = T_HEAP_LOC;
			assert(def_func);
			def_func->addHeapLocal(sym);

			ScmFunc * curr_func = DPC<ScmFunc>(env->context).get();
			assert(curr_func);

			// The current function is accessing an object
			// from different non-global environment.
			// We must set the corresponding flag:
			curr_func->has_closure = true;

			shared_ptr<ScmEnv> p_env = env->parent_env;
			while (p_env) {
				shared_ptr<ScmFunc> p_func = DPC<ScmFunc>(p_env->context);
				if (p_func) {
					if (p_func.get() == def_func) break;

					p_func->has_closure = true;
					p_func->passing_closure = true;
				}
				p_env = p_env->parent_env;
			}
		}
		else {
			sym->location = T_STACK_LOC;
		}
		sym->defined_in_func = def_func;

		P_ScmObj ref = make_shared<ScmRef>(last_sym_name, sym, num_of_levels_up);
		ref->defined_in_func = env->defInFunc();

		return ref;
	}

	P_ScmObj ScmCons::CT_Eval(P_ScmEnv env) {
		car = car->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}
		cdr = cdr->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}

		return shared_from_this();
	}

	ostream &ScmCons::printSrc(ostream &os) const {
		os << "(";
		printElems(os);
		os << ")";
		return os;
	}

	ostream &ScmCons::printElems(ostream &os) const {
		const ScmCons * lst_end = this;
		bool first = true;

		while (lst_end) {
			if (first) first = false;
			else {
				os << " ";
			}
			lst_end->car->printSrc(os);
			//os << endl;
			lst_end = dynamic_cast<ScmCons*>(lst_end->cdr.get());
			// TODO: handle degenerate lists
		}

		return os;
	}


	P_ScmObj ScmFunc::CT_Eval(P_ScmEnv env) {
		if (arg_list && body_list) {
			// Create a new environment for the function
			// and save it in the object.
			// We will work with it during code generation.
			// ScmEnv must hold a reference to its corresponding function
			// because when accessing variables from closures, we need to know where they are defined.
			P_ScmEnv new_env = make_shared<ScmEnv>(env->prog, env);
			new_env->context = shared_from_this();

			// Bind all argument names to ScmArg - we need to tell them apart from unbound variables.
			if (arg_list->t == T_CONS) {
				DPC<ScmCons>(arg_list)->each([&new_env](P_ScmObj &e) {
					P_ScmObj arg = make_shared<ScmArg>();
					new_env->set(e, arg);
					// We want to have ScmRefs in the formal arg_list so that codegen
					// could store the right LLVM Values to each ScmArg
					//e = e->CT_Eval(new_env);
					shared_ptr<ScmSym> argsym = DPC<ScmSym>(e);
					assert(argsym);
					// Create non-weak reference
					e = make_shared<ScmRef>(argsym->val, arg, 0, false);
				});
			}

			// Eval function bodies in the function environment
			assert(body_list->t == T_CONS);
			DPC<ScmCons>(body_list)->each([&new_env](P_ScmObj & e) {
				e = e->CT_Eval(new_env);
			});
			if (new_env->fail()) {
				return nullptr;
			}

			if (env->isGlobal()) {
				location = T_GLOB;
			}
		}
		return shared_from_this();
	}

	ostream &ScmFunc::printSrc(ostream &os) const {
		os << "(lambda ";
		arg_list->printSrc(os);
		os << " ";
		DPC<ScmCons>(body_list)->printElems(os);
		os << ")";
		return os;
	}


	P_ScmObj ScmCall::CT_Eval(P_ScmEnv env) {
		P_ScmObj obj;
		shared_ptr<ScmRef> fref;
		fexpr = fexpr->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}
		fref = DPC<ScmRef>(fexpr);

		int32_t argc_given = arg_list->t == T_NULL ? 0 : DPC<ScmCons>(arg_list)->length();
		argc = argc_given;

		// After evaluation of fexpr, it can either be ScmSym bound to ScmFunc
		// or some ScmExpr (Quote, If, Let). It cannot be Lambda because that is reduced
		// to the first case after fexpr->CT_Eval. We know for certain that Quote won't
		// be a valid case as it always returns data.
		if (fref) {
			obj = fref->refObj();
			if (!obj) {
				// Call to forward referenced object
				// We need to run CT_Eval on this node again
				// when the object is defined.
				env->evalAgain().emplace(
					make_pair(
						fref.get(), ScmEnv::CapturedObj{env, shared_from_this()}
					)
				);
				D(cerr << "queued CT_Eval" << endl);
				return shared_from_this();
			}
		}
		else {
			obj = fexpr;
		}

		if (obj->t == T_FUNC) {
			// Function is known at compilation time - we can hardcode its pointer
			ScmFunc * fn = DPC<ScmFunc>(obj).get();
			int32_t argc_expected = fn->argc_expected;

			if (argc_expected != ArgsAnyCount && argc_given != argc_expected) {
				stringstream ss;
				ss << "Function " << fn->name << " expects " << argc_expected << " arguments, " << argc_given << " given.";
				env->error(ss.str());
				return nullptr;
			}

			indirect = false;
		}
		else if ((obj->t == T_EXPR || obj->t == T_ARG || obj->t == T_CALL) && !DPC<ScmQuoteSyntax>(obj)) {
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
		if (env->fail()) {
			return nullptr;
		}

		return shared_from_this();
	}

	ostream &ScmCall::printSrc(ostream &os) const {
		os << "(";
		fexpr->printSrc(os);
		os << " ";
		if (arg_list->t == T_CONS) {
			DPC<ScmCons>(arg_list)->printElems(os);
		}
		os << ")";
		return os;
	}


	P_ScmObj ScmDefineVarSyntax::CT_Eval(P_ScmEnv env) {
		ScmLambdaSyntax * lambda = DPC<ScmLambdaSyntax>(val).get();

		defined_in_func = DPC<ScmFunc>(env->context).get();

		// In case of lambda inside define which was written explicitly in the
		// source code, we don't want to CT_Eval the lambda function the usual way.
		// Instead, we just convert it to a named function in place.
		if (lambda && !StringRef(DPC<ScmSym>(name)->val).startswith("__lambda#")) {
			const string & fname = DPC<ScmSym>(name)->val;
			ScmCons * c_arg_list = DPC<ScmCons>(lambda->arg_list).get();
			int32_t argc = c_arg_list ? c_arg_list->length() : 0;

			val = make_shared<ScmFunc>(
					argc, fname,
					move(lambda->arg_list), move(lambda->body_list)
			);
		}

		val = val->CT_Eval(env);

		if (env->fail()) {
			return nullptr;
		}

		// Bind symbol to value
		env->set(name, val);

		// Resolve forward references to this newly defined symbol
		auto urefs = env->getUnRefs().equal_range(*DPC<ScmSym>(name));
		for (auto it = urefs.first; it != urefs.second; ++it) {
			ScmSym & sym = const_cast<ScmSym&>(it->first);
			P_ScmObj obj = sym.CT_Eval(it->second.env);
			if (env->fail()) {
				return nullptr;
			}
			shared_ptr<ScmRef> ref = DPC<ScmRef>(obj);
			assert(ref);
			*it->second.ref = *ref;

			auto objs = env->evalAgain().equal_range(it->second.ref.get());
			D(cerr << "got queued objects" << endl);
			for (auto o = objs.first; o != objs.second; ++o) {
				D(cerr << "calling CT_Eval again" << endl);
				o->second.obj->CT_Eval(o->second.env);
				D(cerr << "called CT_Eval again" << endl);
			}
			env->evalAgain().erase(objs.first, objs.second);
		}
		env->getUnRefs().erase(urefs.first, urefs.second);

		if (env->isGlobal()) {
			val->location = T_GLOB;
		}

		return shared_from_this();
	}

	ostream &ScmDefineVarSyntax::printSrc(ostream &os) const {
		os << "(define ";
		name->printSrc(os);
		os << " ";
		val->printSrc(os);
		os << ")";
		return os;
	}


	P_ScmObj ScmDefineFuncSyntax::CT_Eval(P_ScmEnv env) {
		// Converts to ScmDefineVarSyntax with ScmFunc.
		// Then calls CT_Eval on the new object and return it.
		const string & fname = DPC<ScmSym>(name)->val;
		ScmCons * c_arg_list = DPC<ScmCons>(arg_list).get();
		int32_t argc = c_arg_list ? c_arg_list->length() : 0;

		P_ScmObj func = make_shared<ScmFunc>(
				argc, fname,
				move(arg_list), move(body_list)
		);

		// We need to create this binding before function body
		// evaluation in order to have recursion working.
		env->set(name, func);

		P_ScmObj def_var = make_shared<ScmDefineVarSyntax>(name, func);
		def_var = def_var->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}

		return def_var;
	}

	ostream &ScmDefineFuncSyntax::printSrc(ostream &os) const {
		os << "(define (";
		name->printSrc(os);
		os << " ";
		DPC<ScmCons>(arg_list)->printElems(os);
		os << ") ";
		DPC<ScmCons>(body_list)->printElems(os);
		os << ")";
		return os;
	}


	P_ScmObj ScmLambdaSyntax::CT_Eval(P_ScmEnv env) {
		// Creates a new symbol (name) for the anonymous function.
		string fname = env->getUniqID("__lambda#");
		P_ScmObj fsym = make_shared<ScmSym>(fname);
		ScmCons * c_arg_list = DPC<ScmCons>(arg_list).get();
		int32_t argc = c_arg_list ? c_arg_list->length() : 0;

		// Converts this object to ScmDefineVarSyntax.
		P_ScmObj func = make_shared<ScmFunc>(
				argc, fname,
				move(arg_list), move(body_list)
		);
		P_ScmObj def_var = make_shared<ScmDefineVarSyntax>(fsym, func);
		// Prepends the definition to env->prog, runs CT_Eval on the definition.
		def_var = def_var->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}
		env->prog->insert(env->prog_begin, def_var);

		// We've moved the inplace lambda definition to a separate node
		// at the start of ScmProg and now we just return the unique symbol bound to it.
		P_ScmObj ref = make_shared<ScmRef>(fname, func);
		if (env->context) {
			ref->defined_in_func = DPC<ScmFunc>(env->context).get();
		}
		return ref;
	}

	ostream &ScmLambdaSyntax::printSrc(ostream &os) const {
		os << "(lambda ";
		arg_list->printSrc(os);
		os << " ";
		DPC<ScmCons>(body_list)->printElems(os);
		os << ")";
		return os;
	}

	ostream &ScmAndSyntax::printSrc(ostream & os) const {
		ScmCons * el = DPC<ScmCons>(expr_list).get();
		if (!el) {
			os << "(and)";
		}
		else {
			os << "(and ";
			el->printElems(os);
			os << ")";
		}
		return os;
	}

	P_ScmObj ScmAndSyntax::CT_Eval(P_ScmEnv env) {
		expr_list = expr_list->CT_Eval(env);
		return shared_from_this();
	}

	ostream &ScmOrSyntax::printSrc(ostream & os) const {
		ScmCons * el = DPC<ScmCons>(expr_list).get();
		if (!el) {
			os << "(or)";
		}
		else {
			os << "(or ";
			el->printElems(os);
			os << ")";
		}
		return os;
	}

	P_ScmObj ScmOrSyntax::CT_Eval(P_ScmEnv env) {
		expr_list = expr_list->CT_Eval(env);
		return shared_from_this();
	}


	P_ScmObj ScmIfSyntax::CT_Eval(P_ScmEnv env) {
		// TODO: detect dead branches (although that could handle LLVM for us)
		cond_expr = cond_expr->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}
		then_expr = then_expr->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}
		else_expr = else_expr->CT_Eval(env);
		if (env->fail()) {
			return nullptr;
		}
		return shared_from_this();
	}

	ostream &ScmIfSyntax::printSrc(ostream &os) const {
		os << "(if ";
		cond_expr->printSrc(os);
		os << " ";
		then_expr->printSrc(os);
		os << " ";
		else_expr->printSrc(os);
		os << ")";
		return os;
	}


	P_ScmObj ScmLetSyntax::CT_Eval(P_ScmEnv env) {
		P_ScmEnv let_env = make_shared<ScmEnv>(env->prog, env);

		defined_in_func = DPC<ScmFunc>(env->context).get();

		if (bind_list->t != T_NULL) {
			assert(bind_list->t == T_CONS);
			// Populates new environment according to bind_list.
			DPC<ScmCons>(bind_list)->each([&let_env, &env](P_ScmObj e) {
				assert(e->t == T_CONS);
				shared_ptr<ScmCons> kv = DPC<ScmCons>(e);
				P_ScmObj id = kv->car;

				assert(kv->cdr->t == T_CONS);
				P_ScmObj &expr = DPC<ScmCons>(kv->cdr)->car;
				expr = expr->CT_Eval(env);
				let_env->set(id, expr);
			});
			if (env->fail()) {
				return nullptr;
			}
		}
		// Evals body_list in the new environment.
		body_list = body_list->CT_Eval(let_env);
		if (let_env->fail()) {
			return nullptr;
		}

		return shared_from_this();
	}

	ostream &ScmLetSyntax::printSrc(ostream &os) const {
		os << "(let ";
		bind_list->printSrc(os);
		os << " ";
		DPC<ScmCons>(body_list)->printElems(os);
		os << ")";
		return os;
	}

	ScmConsFunc::ScmConsFunc() : Visitable(2, RuntimeSymbol::cons) {}

	ScmCarFunc::ScmCarFunc() : Visitable(1, RuntimeSymbol::car) {}

	ScmCdrFunc::ScmCdrFunc() : Visitable(1, RuntimeSymbol::cdr) {}

	ScmNullFunc::ScmNullFunc() : Visitable(1, RuntimeSymbol::is_null) {}

	ScmPlusFunc::ScmPlusFunc() : Visitable(ArgsAnyCount, RuntimeSymbol::plus) {}

	ScmMinusFunc::ScmMinusFunc() : Visitable(ArgsAnyCount, RuntimeSymbol::minus) {}

	ScmTimesFunc::ScmTimesFunc() : Visitable(ArgsAnyCount, RuntimeSymbol::times) {}

	ScmDivFunc::ScmDivFunc() : Visitable(ArgsAnyCount, RuntimeSymbol::div) {}

	ScmGtFunc::ScmGtFunc() : Visitable(2, RuntimeSymbol::gt) {}

	ScmDisplayFunc::ScmDisplayFunc() : Visitable(1, RuntimeSymbol::display) {}

	ScmNumEqFunc::ScmNumEqFunc() : Visitable(2, RuntimeSymbol::num_eq) {}

	ScmCmdArgsFunc::ScmCmdArgsFunc() : Visitable(0, RuntimeSymbol::cmd_args) {}

	ScmVecLenFunc::ScmVecLenFunc() : Visitable(1, RuntimeSymbol::vec_len) {}

	ScmVecRefFunc::ScmVecRefFunc() : Visitable(2, RuntimeSymbol::vec_ref) {}

	ScmApplyFunc::ScmApplyFunc() : Visitable(2, RuntimeSymbol::apply) {}

	ScmLengthFunc::ScmLengthFunc() : Visitable(1, RuntimeSymbol::length){}

	ostream & ScmRequire::print(ostream & os, int tabs) const {
		printTabs(os, tabs);
		os << "require:" << endl;
		lib_name->print(os, tabs + 1);
		return os;
	}

	ostream & ScmRequire::printSrc(ostream & os) const {
		os << "(require ";
		lib_name->printSrc(os);
		os << ")";
		return os;
	}

	P_ScmObj ScmRequire::CT_Eval(P_ScmEnv env) {
		LibReader librd;
		Metadata input_meta;
		void * metainfo_blob;
		string lib_name_str = DPC<ScmStr>(lib_name).get()->val;

		if (!StringRef(lib_name_str).endswith(".so")) {
			lib_name_str += ".so";
		}

		if (env->link_lib) {
			D(cerr << "Linking the library..." << endl);
			// If compiling "require" in REPL, we must actually link the requested library
			string errmsg;
			sys::DynamicLibrary dylib = sys::DynamicLibrary::getPermanentLibrary(lib_name_str.data(), &errmsg);
			if (dylib.isValid()) {
				metainfo_blob = dylib.getAddressOfSymbol("__llscheme_metainfo__");
				if (!metainfo_blob) {
					env->error("The requested library does not contain any metainfo.");
					return shared_from_this();
				}
			}
			else {
				env->error("Could not load the requested library.");
				return shared_from_this();
			}
		}
		else {
			auto res = getLibraryPath(lib_name_str);
			if (!res.second) {
				env->error("Requested library not found.");
				return shared_from_this();
			}

			if (!librd.load(res.first)) {
				env->error("Could not load the requested library.");
				return shared_from_this();
			}

			metainfo_blob = librd.getAddressOfSymbol("__llscheme_metainfo__");
			if (!metainfo_blob) {
				env->error("The requested library does not contain any metainfo.");
				return shared_from_this();
			}
		}

		if (!input_meta.loadFromBlob(metainfo_blob)) {
			env->error("Invalid metadata in the runtime library.");
			return shared_from_this();
		}

		input_meta.foreachRecord([env](FunctionInfo *rec) {
			D(cerr << "Found function \"" << rec->name << "\" with " << rec->argc << " args." << endl);
			// Add the function into environment
			env->set(rec->name, make_shared<ScmFunc>(rec->argc, rec->name));
		});

		return shared_from_this();
	}
}
