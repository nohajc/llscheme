#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/raw_ostream.h>
#include "../include/driver.hpp"
#include "../include/environment.hpp"
#include "../include/codegen.hpp"
#include "../include/optionparser/argtypes.h"

using namespace std;
using namespace llvm;

namespace llscm {
	enum optionIdx { UNKNOWN, HELP, INPUT_STR, OUTPUT, FILETYPE, BUILDTYPE, OPTLEVEL };
	const option::Descriptor usage[] = {
			{ UNKNOWN, 0, "", "", Arg::Unknown,
					"USAGE: schemec [options] [input file]\n\nOptions:" },
			{ HELP, 0, "", "help", Arg::None,
					"  \t--help  \tPrint usage and exit." },
			{ INPUT_STR, 0, "s", "string", Arg::Required,
					"  -s <str>, \t--string=<str>  \tString containing the input source code." },
			{ OUTPUT, 0, "o", "", Arg::Required,
					"  -o <name> \t  \tOutput file name."},
			{ FILETYPE, 0, "f", "filetype", Arg::Required,
					"  -f <type>, \t--filetype=<type>  \tOutput file type: asm, obj or null." },
			{ BUILDTYPE, 0, "b", "buildtype", Arg::Required,
					"  -b <type>, \t--buildtype=<type>  \tBuild type: exec or lib." },
			{ OPTLEVEL, 0, "O", "", Arg::Numeric,
					"  -O <num> \t  \tOptimization level: -O0, -O1, -O2, -O3." },
			{ 0, 0, 0, 0, 0, 0 }
	};

	const char * Options::ft_id[] = { "asm", "obj", "null" };
	const char * Options::bt_id[] = { "exec", "lib" };

	unique_ptr<Options> getOptions(const vector<option::Option> & cmdargs, option::Parser & parser) {
		unique_ptr<Options> opts = make_unique<Options>();

		if (!cmdargs[INPUT_STR] && parser.nonOptionsCount() != 1) {
			cerr << "Error: Input file or string missing." << endl;
			opts->invalid = true;
			return opts;
		}

		if (cmdargs[INPUT_STR] && parser.nonOptionsCount() >= 1) {
			cerr << "Error: Provide either input file or string. Not both." << endl;
			opts->invalid = true;
			return opts;
		}

		if (cmdargs[INPUT_STR]) {
			opts->setStringInput(cmdargs[INPUT_STR].arg);
		}
		else {
			StringRef input_fname = parser.nonOption(0);
			if (input_fname != "-" && !input_fname.endswith(".scm")) {
				cerr << "Expected \".scm\" input file or \"-\" (standard input)." << endl;
				opts->invalid = true;
				return opts;
			}
			opts->setFileInput(input_fname);
		}

		if (cmdargs[OUTPUT]) {
			opts->setOutputName(cmdargs[OUTPUT].arg);
		}

		if (cmdargs[FILETYPE]) {
			D(cerr << "Option: filetype=" << cmdargs[FILETYPE].arg << endl);
			opts->setOutputType(cmdargs[FILETYPE].arg);
		}

		if (cmdargs[BUILDTYPE]) {
			opts->setBuildType(cmdargs[BUILDTYPE].arg);
		}

		if (cmdargs[OPTLEVEL]) {
			opts->setOptLevel(cmdargs[OPTLEVEL].arg);
		}

		return opts;
	}

	void Driver::writeBitcodeToPipe(Module * mod, FILE * pipe) {
		int pipe_fd;
		unique_ptr<raw_ostream> pipe_stream;

		pipe_fd = fileno(pipe);
		pipe_stream = make_unique<raw_fd_ostream>(pipe_fd, false);

		WriteBitcodeToFile(mod, *pipe_stream);
		fflush(pipe);
	}

	string Driver::getLLCompilerCommand() {
		stringstream ss;

		ss << "llc -";

		if (opts->has_output_name) {
			ss << " -o " << opts->out_fname;
		}

		ss << " -filetype=" << opts->ft_id[opts->filetype];
		ss << " -O" << opts->optlevel;

		return ss.str();
	}

	bool Driver::invokeLLC(Module * mod) {
		string command;
		FILE * pipe;

		command = getLLCompilerCommand();
		D(cerr << "Command: " << command << endl);

		pipe = popen(command.c_str(), "w");
		writeBitcodeToPipe(mod, pipe);
		pclose(pipe);

		return true;
	}

	bool Driver::compile(unique_ptr<Parser> && p) {
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
		if (opts->buildtype == Options::BT_EXEC) {
			cg.makeExecutable();
		} // Otherwise we're building a library (module without main function)
		cg.run();
		cg.dump();

		invokeLLC(cg.getModule());

		return true;
	}

	bool Driver::compileSourceFile(const string & fname) {
		bool st;

		if (fname == "-") {
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

	bool Driver::compileString(const string & str) {
		unique_ptr<Reader> r = make_unique<StringReader>(str);
		return compile(make_unique<Parser>(r));
	}

	bool Driver::run() {
		if (opts->has_string_input) {
			return compileString(opts->source_str);
		}
		return compileSourceFile(opts->in_fname);
	}
}

int main(int argc, char * argv[]) {
	using namespace llscm;

	option::Stats stats(true, usage, argc - 1, argv + 1);
	vector<option::Option> cmdargs(stats.options_max);
	vector<option::Option> buffer(stats.buffer_max);
	option::Parser parser(true, usage, argc - 1, argv + 1, &cmdargs[0], &buffer[0]);

	if (parser.error()) {
		return EXIT_FAILURE;
	}

	if (cmdargs[HELP] || argc <= 1) {
		int columns = getenv("COLUMNS") ? atoi(getenv("COLUMNS")) : 80;
		option::printUsage(cout, usage, columns);
		return EXIT_SUCCESS;
	}

	auto opts = getOptions(cmdargs, parser);
	if (opts->invalid) {
		return EXIT_FAILURE;
	}

	Driver driver(move(opts));

	if (!driver.run()) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
