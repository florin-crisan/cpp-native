#include <fcrisan/native/text_writer.hpp>
#include <fcrisan/native/file.hpp>
#include <fcrisan/native/text.hpp>
#include <fcrisan/native/error.hpp>
#include <stdexcept>
#include <safeint.h>
#include <Windows.h>

template <typename T>
using safe_int = msl::utilities::SafeInt<T>;

namespace fcrisan::native {

	struct text_writer::impl {
	public:
		impl(file &f, code_page_id cp)
			:
			outFile{ f },
			outCp{ cp },
			isNull{ f.handle() == nullptr }
		{
			isConsole = is_console(f);
			isLegacyCp = (cp != code_pages::utf8) && (cp != code_pages::utf16);
			doPrecompose = isLegacyCp || isConsole;
		}

		void write(const char *what, std::size_t count, code_page_id cp) {
			if (!what)
				throw std::invalid_argument("bad string pointer passed to text_writer::write");
			if (!count)
				return; // nothing to write;
			if (isNull)
				return;

			auto wideStr = to_wide_string(what, count, cp);
			write(&(wideStr[0]), wideStr.size());
		}

		void write(const wchar_t *what, std::size_t count) {
			if (!what)
				throw std::invalid_argument("bad string pointer passed to text_writer::write");
			if (!count)
				return; // nothing to write;
			if (isNull)
				return;

			std::wstring precomposed; // declare here so pointer to it is valid in the whole function
			if (doPrecompose) {
				precomposed = precompose(what, count);
				if (isLegacyCp && !isConsole)
					replace_legacy_chars(&(precomposed[0]), precomposed.size());
				what = &(precomposed[0]);
				count = precomposed.size();
			}

			auto wideString = normalize_eol(what, count, L"\r\n");

			if (isConsole) {
				// The console holds the text internally as Unicode. So no point in writing to it as it it were a file (that would be in the console's output code page).
				write_to_console(&(wideString[0]), wideString.size());
				return;
			}

			if (outCp == code_pages::utf16) {
				auto byteCount = safe_int<std::size_t>(wideString.size()) * sizeof(wchar_t);
				outFile.write(&(wideString[0]), byteCount);
				return;
			}

			auto narrowString = to_narrow_string(&(wideString[0]), wideString.size(), outCp);
			outFile.write(&(narrowString[0]), narrowString.size());
		}

	private:
		void write_to_console(const wchar_t *what, std::size_t count) {
			DWORD written;
			safe_int<DWORD> toWrite{ count };
			clear_error();
			bool ok = ::WriteConsoleW(outFile, what, toWrite, &written, nullptr);
			if (!ok || written != toWrite)
				throw_error("Cannot write to file");
		}

		bool is_console(file_handle f) {
			if (!f)
				return false;
			DWORD consoleMode;
			clear_error();
			if (::GetConsoleMode(f, &consoleMode))
				return true;
			auto err = last_error();
			if (err != ERROR_INVALID_HANDLE)
				throw_error("Cannot get console mode", err);
			return false;
		}

		file &outFile;
		code_page_id outCp;
		bool isNull;
		bool doPrecompose;
		bool isLegacyCp;
		bool isConsole;
	};

	text_writer::text_writer(file &outFile, code_page_id cp)
		: pimpl(new impl(outFile, cp))
	{
	}

	text_writer::~text_writer() {
	}

	void text_writer::write(const char *what, std::size_t count, code_page_id cp) {
		pimpl->write(what, count, cp);
	}

	void text_writer::write(const wchar_t *what, std::size_t count) {
		pimpl->write(what, count);
	}
}
