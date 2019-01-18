#include <iostream>
#include <vector>
#include <regex>
#include <future>
#include <fcrisan/native/error.hpp>
#include <fcrisan/native/filebuf.hpp>
#include <fcrisan/native/file.hpp>
#include <fcrisan/native/text_writer.hpp>

#include <Windows.h>
#undef min
#undef max

#include <SafeInt.hpp>

using namespace fcrisan::native;
template <typename T> using safe_int = SafeInt<T>;

namespace  {

union ZString {
	const char *outputa;
	const wchar_t *outputw;

	ZString(const char *a) :outputa(a) {}
	ZString(const wchar_t *w) :outputw(w) {}
};

template <typename Ch>
struct TestCase {
	const char * name;
	const Ch * input;
	fcrisan::native::code_page_id codePage;
	ZString zstring;
	std::wstring consoleOut;
};

std::vector<TestCase<char>> testCasesA {
#define INPUT_STRING(x) u8 ## x
#include "test_cases.inline"
};

std::vector<TestCase<wchar_t>> testCasesW {
#undef INPUT_STRING
#define INPUT_STRING(x) L ## x
#include "test_cases.inline"
};

HANDLE GetStdOut() {
	clear_error();
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE)
		throw_error("Cannot get standard output handle");
	if (!hStdOut)
		throw_error("No standard output is enabled (graphical Windows application?)");
	return hStdOut;
}

void ClearScreen() {
	auto hStdOut = GetStdOut();

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	clear_error();
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi))
		throw_error("Cannot get console screen buffer info (is output redirected?)");

	auto cellsToWrite = safe_int<DWORD>(csbi.dwSize.X) * csbi.dwSize.Y;
	COORD startCoord = { 0, 0 };
	DWORD written;
	clear_error();
	if (!FillConsoleOutputCharacterW(hStdOut, L' ', cellsToWrite, startCoord, &written))
		throw_error("Cannot clear screen");
	if (written != cellsToWrite)
		throw_error("Cannot clear screen");

	clear_error();
	if (!SetConsoleCursorPosition(hStdOut, startCoord))
		throw_error("Cannot reposition cursor");
}

std::wstring GetConsoleText() {
	auto hStdOut = GetStdOut();

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	clear_error();
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi))
		throw_error("Cannot get console screen buffer info");

	std::wstring buffer;

	for (SHORT lineNo = 0; lineNo <= csbi.dwCursorPosition.Y; ++lineNo) {
		SHORT cellsToRead = (lineNo == csbi.dwCursorPosition.Y) ? csbi.dwCursorPosition.X : csbi.dwSize.X;

		COORD startCoord = { 0, lineNo };

		std::wstring line;
		line.resize(safe_int<size_t>(cellsToRead));
		DWORD charsRead;
		clear_error();
		if (!ReadConsoleOutputCharacterW(hStdOut, &line[0], safe_int<DWORD>(cellsToRead), startCoord, &charsRead))
			throw_error("Cannot read screen buffer");
		line.resize(charsRead);

		static const std::wregex trimEol(L" +$");
		line = std::regex_replace(line, trimEol, L"");

		if (lineNo != 0)
			buffer.append(L"\n");
		buffer.append(line);
	}

	return buffer;
}

std::vector<std::byte> read_to_end(file &f) {
	std::vector<std::byte> bytes;
	bytes.resize(4096); // Should be enough for our tests.
	auto bytesRead = f.read(&(bytes[0]), bytes.size());
	bytes.resize(bytesRead);
	return bytes;
}

template <typename Ch, typename It>
int CompareResults(const TestCase<Ch> &tc, const It &begin, const It &end) {
	const std::byte *pStart;
	const std::byte *pEnd;

	if (tc.codePage == code_pages::utf16) {
		pStart = reinterpret_cast<const std::byte *>(tc.zstring.outputw);
		pEnd = reinterpret_cast<const std::byte *>(tc.zstring.outputw + wcslen(tc.zstring.outputw));
	}
	else {
		pStart = reinterpret_cast<const std::byte *>(tc.zstring.outputa);
		pEnd = reinterpret_cast<const std::byte *>(tc.zstring.outputa + strlen(tc.zstring.outputa));
	}

	auto expectedSize = safe_int<std::size_t>(pEnd - pStart);
	auto fileSize = safe_int<std::size_t>(end - begin);

	if (fileSize != expectedSize)
		return 1;

	if (!std::equal(begin, end, pStart))
		return 1;

	return 0;
}

template <typename Ch>
int CompareResults(const TestCase<Ch> &tc, const std::vector<std::byte> &fileContents) {
	return CompareResults(tc, fileContents.begin(), fileContents.end());
}

template <typename Ch>
int CompareResults(const TestCase<Ch> &tc, const std::wstring &result) {
	if (result.size() != tc.consoleOut.size())
		return 1;
	if (!std::equal(result.begin(), result.end(), tc.consoleOut.begin()))
		return 1;
	return 0;
}

template <typename Ch>
int RunPipeTest(const TestCase<Ch> &tc) {
	using namespace std;
	using namespace fcrisan::native;

	auto pipe = create_anonymous_pipe();

	auto w = text_writer{ *pipe.write, tc.codePage };
	auto buf = new fcrisan::native::filebuf<Ch>{ w };
	auto out = std::basic_ostream<Ch>{buf};

	std::vector<std::byte> fileContents;

	auto readTask = std::async(std::launch::async, [&]() {
		fileContents = read_to_end(*pipe.read);
		pipe.read->close();
	});
	auto writeTask = std::async(std::launch::async, [&]() {
		out << tc.input;
		out.flush();
		pipe.write->close();
	});
	readTask.get();
	writeTask.get();
	return CompareResults<Ch>(tc, fileContents);
}

template <typename Ch>
int RunFileTest(const TestCase<Ch> &tc) {
	using namespace std;
	using namespace fcrisan::native;

	clear_error();
	HANDLE h = ::CreateFileW(L"test_output.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	if (h == INVALID_HANDLE_VALUE)
		throw_error("Cannot create temp file");
	
	auto f = file{ h, true };
	auto w = text_writer{ f, tc.codePage };
	auto buf = new fcrisan::native::filebuf<Ch>{ w };
	auto out = std::basic_ostream<Ch, std::char_traits<Ch>>{buf};

	out << tc.input;
	out.flush();

	f.rewind();

	auto fileContents = read_to_end(f);
	return CompareResults<Ch>(tc, fileContents);
}

template <typename Ch>
int RunNullTest(const TestCase<Ch> &tc) {
	using namespace std;
	using namespace fcrisan;

	auto f = file{ nullptr, false }; // Empty writer.
	auto w = text_writer{ f, tc.codePage };
	auto buf = new fcrisan::native::filebuf<Ch>{ w };
	auto out = std::basic_ostream<Ch, std::char_traits<Ch>>{ buf };

	out << tc.input;
	out.flush();

	// Nothing should get written. We just want to make sure it works (i.e. it doesn't crash) when there is not standard output.

	return 0;
}

template <typename Ch>
int RunConsoleTest(const TestCase<Ch> &tc) {
	using namespace std;
	using namespace fcrisan;

	auto f = file{ GetStdOut(), false };
	auto w = text_writer{ f, tc.codePage };
	auto buf = new fcrisan::native::filebuf<Ch>{ w };
	auto out = std::basic_ostream<Ch, std::char_traits<Ch>>{ buf };

	ClearScreen();
	out << tc.input;
	out.flush();

	auto consoleText = GetConsoleText();
	return CompareResults<Ch>(tc, consoleText);
}

template <typename Ch>
int RunTests(const std::vector<TestCase<Ch>> &testCases, std::function<int (const TestCase<Ch> &)> fn, const char *destination) {
	int failed = 0;
	for (const auto &tc : testCases) {
		int result = fn(tc);
		failed += result;

		if (result)
			std::cerr << "Test case " << tc.name << " failed on " << destination << "\n";
	}
	return failed;
}

} // anonymous namespace

int main() {
	using namespace std;
	using namespace fcrisan::native;
	try {
		clear_error();
		HANDLE h = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			throw_error("Cannot get console output buffer");
		file hOut{ h, true };

		// Reset standard output to console output buffer, just in case it was redirected elsewhere.
		clear_error();
		if (!::SetStdHandle(STD_OUTPUT_HANDLE, hOut))
			throw_error("Cannot set standard output handle");

		int failed = 0;
		failed += RunTests<char>(testCasesA, &RunNullTest<char>, "null");
		failed += RunTests<wchar_t>(testCasesW, &RunNullTest<wchar_t>, "null");
		failed += RunTests<char>(testCasesA, &RunFileTest<char>, "file");
		failed += RunTests<wchar_t>(testCasesW, &RunFileTest<wchar_t>, "file");
		failed += RunTests<char>(testCasesA, &RunPipeTest<char>, "pipe");
		failed += RunTests<wchar_t>(testCasesW, &RunPipeTest<wchar_t>, "pipe");
		failed += RunTests<char>(testCasesA, &RunConsoleTest<char>, "console");
		failed += RunTests<wchar_t>(testCasesW, &RunConsoleTest<wchar_t>, "console");
		cerr << failed << " tests failed\n";
		return failed;
	}
	catch (const exception &ex) {
		cerr << "[" << typeid(ex).name() << "] " << ex.what() << endl;
		return -1;
	}
}
