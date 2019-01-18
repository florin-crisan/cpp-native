#include <fcrisan/native/text.hpp>
#include <fcrisan/native/error.hpp>
#include <SafeInt.hpp>
#include <Windows.h>
#undef min
#undef max

template <typename T> using safe_int = SafeInt<T>;

namespace fcrisan::native {

	static_assert(sizeof(code_page_id) == sizeof(UINT));

	namespace  {

		int do_widen(code_page_id codePage, const char *originalString, int originalCount, wchar_t *convertedString, int convertedSize) {
			return ::MultiByteToWideChar(codePage, 0, originalString, originalCount, convertedString, convertedSize);
		}

		int do_narrow(code_page_id codePage, const wchar_t *originalString, int originalCount, char *convertedString, int convertedSize) {
			return ::WideCharToMultiByte(codePage, 0, originalString, originalCount, convertedString, convertedSize, nullptr, nullptr);
		}

		template <typename To, typename From, typename Fn>
		std::basic_string<To> convert(Fn fn, const From *originalString, int originalCount, code_page_id codePage) {
			clear_error();
			int convertedCount = fn(codePage, originalString, originalCount, nullptr, 0);
			if (!convertedCount)
				throw_error("cannot get converted string length");
			std::basic_string<To> convertedString;
			convertedString.resize(safe_int<std::size_t>(convertedCount));
			int actuallyConverted = fn(codePage, originalString, originalCount, &(convertedString[0]), convertedCount);
			if (!actuallyConverted)
				throw_error("cannot convert string");
			if (actuallyConverted != convertedCount)
				throw_error("partial conversion");
			return convertedString;
		}

	}

	std::wstring to_wide_string(const char *narrowString, code_page_id codePage) {
		return convert<wchar_t>(&do_widen, narrowString, -1, codePage);
	}

	std::wstring to_wide_string(const char *narrowString, std::size_t count, code_page_id codePage) {
		int intCount = safe_int<int>(count);
		return convert<wchar_t>(&do_widen, narrowString, intCount, codePage);
	}

	std::string to_narrow_string(const wchar_t *wideString, std::size_t count, code_page_id codePage) {
		int intCount = safe_int<int>(count);
		return convert<char>(&do_narrow, wideString, intCount, codePage);
	}

	std::string to_narrow_string(const wchar_t *wideString, code_page_id codePage) {
		return convert<char>(&do_narrow, wideString, -1, codePage);
	}

	std::wstring precompose(const wchar_t *what, std::size_t count) {
		safe_int<int> originalSize{ count };
		clear_error();
		safe_int<int> foldedCount = ::FoldStringW(MAP_PRECOMPOSED, &what[0], originalSize, nullptr, 0);
		if (!foldedCount)
			fcrisan::native::throw_error("Cannot get precomposed string length");
		std::wstring foldedString;
		foldedString.resize(foldedCount);
		clear_error();
		safe_int<int> actuallyFolded = ::FoldStringW(MAP_PRECOMPOSED, &what[0], originalSize, &foldedString[0], foldedCount);
		if (!actuallyFolded || actuallyFolded != foldedCount)
			fcrisan::native::throw_error("Cannot convert string to precomposed characters");
		return foldedString;
	}

	void replace_legacy_chars(wchar_t *what, std::size_t count) {
		wchar_t *end = what + count;
		for (wchar_t *ch = what; ch != end; ++ch) {
			if (*ch == L'\u0218') *ch = L'\u015E'; // S-comma -> S-cedilla
			else if (*ch == L'\u0219') *ch = L'\u015F'; // s-comma -> s-cedilla
			else if (*ch == L'\u021A') *ch = L'\u0162'; // T-comma -> T-cedilla
			else if (*ch == L'\u021B') *ch = L'\u0163'; // t-comma -> t-cedilla
		}
	}

	namespace {

		// Make sure we're on Windows, with UTF16.
		static_assert(sizeof(wchar_t) == sizeof(char16_t));

		const wchar_t surrogateMask = 0b1111'1100'0000'0000;
		const wchar_t highSurrogateStart = 0b1101'1000'0000'0000;
		const wchar_t lowSurrogateStart = 0b1101'1100'0000'0000;
		/*
			D    8    0    0    - high surrogate start
			1101 1000 0000 0000
			D    B    F    F    - high surrogate end
			1101 1011 1111 1111
			D    C    0    0    - low surrogate start
			1101 1100 0000 0000
			D    F    F    F    - low surrogate end
			1101 1111 1111 1111
		*/
		char32_t read_char(const wchar_t * &c, const wchar_t *end) {
			if (c == end)
				return 0;
			if ((*c & surrogateMask) == highSurrogateStart) {
				const wchar_t *next = c + 1;
				if (next == end) {
					return *(c++); // Nothing more; return this half-char as a char (invalid in unicode btw)
				}
				if ((*next & surrogateMask) != lowSurrogateStart) {
					// Not followed by low surrogate; just leave it as it is.
					return *(c++);
				}
				char32_t result = static_cast<char32_t>(((*c & ~surrogateMask) << 10) | (*next & surrogateMask));
				c = next + 1;
				return result;
			}
			//if ((*c & surrogateMask) == highSurrogateStart) // low surrogate
			//	continue; // This should have been treated with the high surrogate.
			return *(c++);
		}

	}

	std::wstring normalize_eol(const wchar_t *p, std::size_t count, const wchar_t *eol) {
		const wchar_t * const end = p + count;

		std::wstring result;
		while (p != end) {
			const wchar_t * char_start = p;
			char32_t c = read_char(p, end);
			if (c == 0)
				break;
			const wchar_t * char_end = p;

			if (c == 0x85 /*NEL*/ || c == 0xA /*LF*/) {
				result.append(eol);
				continue;
			}
			if (c == 0xD /*CR*/) {
				const wchar_t *pnext = p;
				char32_t nextChar = read_char(pnext, end);
				if (nextChar != 0) {
					if (nextChar == 0xA /*LF*/) { // skip LF
						p = pnext;
					}
					result.append(eol);
					continue;
				}
			}

			/*
			TODO:
			0xB = VT = vertical tab
			0xC = FF = form feed
			0x2028 = LS = line separator
			0x2029 = PS = paragraph separator
			*/

			result.append(char_start, char_end);
		}
		return result;
	}

}
