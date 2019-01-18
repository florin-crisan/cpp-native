#include <fcrisan/native/error.hpp>
#include <fcrisan/native/file.hpp>
#include <fcrisan/native/text_writer.hpp>
#include <fcrisan/native/filebuf.hpp>
#include <fcrisan/native/ostream_extractors.hpp>
#include <fcrisan/native/text.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <Windows.h>
#undef min
#undef max

int normal_main(int argc, const char **argv) {
	using namespace std;
	for (int i = 1; i < argc; ++i) {
		if (i > 1)
			cout << " ";
		cout << argv[i];
	}
	return 0;
}

int main() {
	using namespace std;
	using namespace fcrisan;

	try {
		cout.exceptions(ios_base::eofbit | ios_base::badbit | ios_base::failbit);

		native::file out{ ::GetStdHandle(STD_OUTPUT_HANDLE), false };
		native::file err{ ::GetStdHandle(STD_ERROR_HANDLE), false };

		native::text_writer outWriter{ out, ::GetConsoleOutputCP() };
		native::text_writer errWriter{ out, ::GetConsoleOutputCP() };

		auto outBuf = new native::filebuf<char>{ outWriter };
		auto errBuf = new native::filebuf<char>{ errWriter };

		cout.rdbuf(outBuf);
		cerr.rdbuf(errBuf);

		const wchar_t * cmdLine = ::GetCommandLineW();

		int argCount;
		auto arguments = ::CommandLineToArgvW(cmdLine, &argCount);
		if (!arguments)
			native::throw_error("cannot get command line");
		auto deleter = [](wchar_t**x) { ::LocalFree(x); };
		shared_ptr<wchar_t*>pargs{ arguments, deleter };

		std::vector<std::string> narrowArgs;
		std::vector<const char *> argv;
		for (int i = 0; i < argCount; ++i)
			narrowArgs.push_back(native::to_narrow_string(arguments[i]));
		for (const auto &a : narrowArgs)
			argv.push_back(&a[0]);

		return normal_main(argCount, &argv[0]);
	}
	catch (const std::exception &ex) {
		cerr << "[" << typeid(ex).name() << "] " << ex.what() << endl;
	}
}
