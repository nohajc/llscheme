#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include "../include/driver.hpp"
#include "../include/parser.hpp"
#include "../include/environment.hpp"
#include "../include/codegen.hpp"
#include "../include/compiler.hpp"

using namespace std;
using namespace llvm;

namespace llscm {
	int argc;
	char ** argv;

	void writeBitcodeToPipe(Module * mod, FILE * pipe) {
		int pipe_fd;
		unique_ptr<raw_ostream> pipe_stream;

		pipe_fd = fileno(pipe);
		pipe_stream = make_unique<raw_fd_ostream>(pipe_fd, false);

		WriteBitcodeToFile(mod, *pipe_stream);
		fflush(pipe);
	}

	bool invokeLLC(Module * mod) {
		stringstream ss;
		string command;
		FILE * pipe;

		StringRef input_fname = argv[1];
		string output_fname;
		if (input_fname == "-" || input_fname == "-s") {
			// TODO: pass the right -o option
			ss << "llc - -filetype=null -O0" << endl;
		}
		else {
			output_fname = input_fname.drop_back(4);
			output_fname += ".o";
			ss << "llc - -filetype=obj -O0 -o " << output_fname << endl;
		}

		//cerr << "Command: " << ss.str();
		command = ss.str();
		pipe = popen(command.c_str(), "w");
		writeBitcodeToPipe(mod, pipe);
		pclose(pipe);

		return true;
	}

	bool compile(unique_ptr<Parser> && p) {
		ScmProg prog = p->NT_Prog();
		shared_ptr<ScmEnv> env = createGlobalEnvironment(prog);

		if (p->fail()) {
			return false;
		}
		for (auto & e: prog) {
			cout << *e;
			e = e->CT_Eval(env);
			if (env->fail()) {
				return false;
			}
		}

		for (auto & e: prog) {
			e->printSrc(cerr);
			cerr << endl;
		}

		ScmCodeGen cg(getGlobalContext(), &prog);
		cg.dump();

		invokeLLC(cg.getModule());

		/*ScmCompiler scm_compiler(cg.getModule(), cg.getContext());
		int err_code = scm_compiler.run();

		return !err_code;*/
		return true;
	}

	bool compileSourceFile(const char * fname) {
		bool st;
		if (!strcmp(fname, "-")) {
			unique_ptr<Reader> r = make_unique<FileReader>(cin);
			st = compile(make_unique<Parser>(r));
		}
		else {
			fstream fs(fname, fstream::in);
			if (fs.fail()) {
				cerr << "Cannot open file " << fname << "." << endl;
				return false;
			}

			unique_ptr<Reader> r = make_unique<FileReader>(fs);
			st = compile(make_unique<Parser>(r));

			fs.close();
		}
		return st;
	}

	bool compileString(const char * str) {
		unique_ptr<Reader> r = make_unique<StringReader>(str);
		return compile(make_unique<Parser>(r));
	}
}

int main(int argc, char * argv[]) {
	llscm::argc = argc;
	llscm::argv = argv;

	if (argc >= 2) {
		if (!strcmp(argv[1], "-s")) {
			if (argc != 3) return EXIT_SUCCESS;

			if (!llscm::compileString(argv[2])) {
				return EXIT_FAILURE;
			}
		}
		else {
			StringRef input_fname = argv[1];
			if (input_fname != "-" && !input_fname.endswith(".scm")) {
				cerr << "Expected \".scm\" input file or \"-\" (standard input)." << endl;
				return EXIT_FAILURE;
			}
			if (!llscm::compileSourceFile(argv[1])) {
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}
