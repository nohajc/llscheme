#include <cstdint>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include "../include/ast.hpp"

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
}
