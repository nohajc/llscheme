#ifndef LLSCHEME_DRIVER_HPP
#define LLSCHEME_DRIVER_HPP

//#include <llvm/ADT/StringRef.h>

#include "parser.hpp"

namespace llscm {
	using namespace llvm;
	using namespace std;

	extern int argc;
	extern char ** argv;

	class Options;

	class Driver {
		unique_ptr<Options> opts;

		bool compileSourceFile(const string & fname);
		bool compileString(const string & str);
		bool compile(unique_ptr<Parser> && p);
		void writeBitcodeToPipe(Module * mod, FILE * pipe);
		string getLLCompilerCommand();
		bool invokeLLC(Module * mod);
	public:
		Driver(unique_ptr<Options> o) : opts(move(o)) {}
		bool run();
	};

	class Options {
		friend class Driver;

		enum Filetype {
			FT_ASM, FT_OBJ, FT_NULL, FT_COUNT
		};

		enum Buildtype {
			BT_EXEC, BT_LIB, BT_COUNT
		};

		static const char * ft_id[];
		static const char * bt_id[];

		bool has_string_input;
		bool has_output_name;
		// File name of the source code
		string in_fname;
		// The actual source code itself
		string source_str;
		// Output file name
		string out_fname;
		// Output file type
		Filetype filetype;
		// Build type
		Buildtype buildtype;
		// Optimization level
		int optlevel;

	public:
		bool invalid;

		Options() {
			invalid = false;
			// Default options
			filetype = FT_OBJ;
			buildtype = BT_EXEC;
			optlevel = 0;
			has_output_name = false;
		}

		Options & setStringInput(const string & str) {
			source_str = str;
			has_string_input = true;
			return *this;
		}

		Options & setFileInput(const string & str) {
			in_fname = str;
			has_string_input = false;

			if (!has_output_name && in_fname != "-") {
				// Set the default output name
				// derived from the input name
				out_fname = StringRef(in_fname).drop_back(4);
				out_fname += ".o";
				has_output_name = true;
			}

			return *this;
		}

		Options & setOutputName(const string & str) {
			out_fname = str;
			has_output_name = true;
			return *this;
		}

		Options & setOutputType(const string & str) {
			for (int i = 0; i < FT_COUNT; i++) {
				cerr << "trying " << ft_id[i] << " with " << str << endl;
				if (str == ft_id[i]) {
					cerr << "FOUND MATCH" << endl;
					filetype = (Filetype)i;
					break;
				}
			}
			return *this;
		}

		Options & setBuildType(const string & str) {
			for (int i = 0; i < BT_COUNT; i++) {
				if (str == bt_id[i]) {
					buildtype = (Buildtype)i;
					break;
				}
			}
			return *this;
		}

		Options & setOptLevel(const string & str) {
			optlevel = atoi(str.c_str());
			return *this;
		}
	};
}

#endif //LLSCHEME_DRIVER_HPP