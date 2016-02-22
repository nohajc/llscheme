#include <cstdint>
#include <memory>
#include <string>

namespace llscm {
	using namespace std;

	extern const int32_t ArgsAnyCount;

	enum scm_type {
		T_INT,
		T_STR,
		T_SYM,
		T_CONS,
		T_TRUE,
		T_FALSE,
		T_NIL,
		T_EOF,
		T_EXPR,
		T_FUNC,
		T_CALL,
		T_VECTOR
	};

	/*
	 * This class hierarchy is used to generate AST.
	 * It is created by parsing the source file.
	 * First we just read atoms and lists creating
	 * corresponding scm_cons cells with symbols, strings, numbers etc.
	 * Then we do the semantic analysis and compile-time evaluation:
	 * Evaluating defines, looking up functions by their names, creating nodes
	 * for lambdas, function calls, expressions and processing macros.
	 */
	class scm_obj {
	public:
		scm_obj(scm_type type) {
			t = type;
		}

		scm_type t;
	};

	typedef shared_ptr<scm_obj> scm_obj_ptr;
	typedef char scm_op;

	class scm_int: public scm_obj {
	public:
		scm_int(int64_t value): scm_obj(T_INT) {
			val = value;
		}

		int64_t val;
	};

	class scm_lit: public scm_obj {
	public:
		scm_lit(scm_type type, const string & value):
			scm_obj(type), val(value) {}

		int32_t length;
		string val;
	};

	class scm_str: public scm_lit {
	public:
		scm_str(const string & value): scm_lit(T_STR, value) {}
	};

	class scm_sym: public scm_lit {
	public:
		scm_sym(const string & value): scm_lit(T_SYM, value) {}
	};

	class scm_cons: public scm_obj {
	public:
		scm_cons(scm_obj_ptr pcar, scm_obj_ptr pcdr):
			scm_obj(T_CONS), car(pcar), cdr(pcdr) {}

		scm_obj_ptr car;
		scm_obj_ptr cdr;
	};

	class scm_expr: public scm_obj {
	public:
		scm_expr(): scm_obj(T_EXPR) {}
	};

	// When args and bodies are set to nullptr, scm_func
	// is a reference to an external runtime library function.
	// There will be a compile-time environment populated with
	// native functions that will also be updated with user definitions.
	class scm_func: public scm_obj {
	public:
		scm_func(const string & fname, int32_t argc, scm_obj_ptr args, scm_obj_ptr bodies):
			scm_obj(T_FUNC), name(fname), arg_list(args), body_list(bodies) {
			argc_expected = argc;
		}

		string name;
		int32_t argc_expected;
		scm_obj_ptr arg_list;
		scm_obj_ptr body_list;
		// scm_env def_env; // TODO
	};

	class scm_call: public scm_obj {
	public:
		scm_call(scm_obj_ptr pfunc, scm_obj_ptr args):
			scm_obj(T_CALL), func(pfunc), arg_list(args) {}

		scm_obj_ptr func;
		scm_obj_ptr arg_list;
	};

	/* Classes derived from scm_inline_call represent primitive functions
	 * such as arithmetic operations for which we want to emit instructions
	 * inline instead of an explicit function call.
	 */
	class scm_inline_call: public scm_call {

	};

	class scm_native_syntax: public scm_expr {

	};

	class scm_define_syntax: public scm_expr {

	};

	class scm_lambda_syntax: public scm_expr {

	};

	class scm_quote_syntax: public scm_expr {

	};

	class scm_if_syntax: public scm_expr {

	};

	class scm_let_syntax: public scm_expr {

	};
}
