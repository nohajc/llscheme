#include <iostream>
#include <sstream>
#include <string>

namespace llscm {
	using namespace std;

	class Reader {
	protected:
		string tok;
		istream * is;
	public:
		const char * nextToken();
		const char * currToken();

		virtual ~Reader() {};
	};

	class FileReader: public Reader {
	public:
		FileReader(istream & f);
	};

	class StringReader: public Reader {
	public:
		StringReader(const string & str);
		virtual ~StringReader();
	};
}
