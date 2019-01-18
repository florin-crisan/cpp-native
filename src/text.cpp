#include <fcrisan/native/text.hpp>
#include <fcrisan/native/error.hpp>
#include <SafeInt.hpp>
#include <Windows.h>
#undef min
#undef max

template <typename T> using safe_int = SafeInt<T>;

namespace fcrisan::native {

	static_assert(sizeof(code_page_id) == sizeof(UINT));

	std::wstring to_wide_string(const char *narrowString, std::size_t count, code_page_id codePage) {
		safe_int<int> narrowCount{ count };
		clear_error();
		safe_int<int> wideCount = ::MultiByteToWideChar(codePage, 0, narrowString, narrowCount, nullptr, 0);
		if (!wideCount)
			throw_error("Cannot get wide string length");
		std::wstring wideString;
		wideString.resize(wideCount);
		clear_error();
		safe_int<int> resultingChars = ::MultiByteToWideChar(codePage, 0, narrowString, narrowCount, &wideString[0], wideCount);
		if (!resultingChars || resultingChars != wideCount)
			throw_error("Cannot convert to wide string");
		return wideString;
	}

	std::string to_narrow_string(const wchar_t *wideString, std::size_t count, code_page_id codePage) {
		safe_int<int> wideCount{ count };
		clear_error();
        safe_int<int> narrowCount = ::WideCharToMultiByte(codePage, 0, wideString, wideCount, nullptr, 0, nullptr, nullptr);
		if (!narrowCount)
			throw_error("Cannot get narrow string length");
		std::string narrowString;
		narrowString.resize(narrowCount);
		clear_error();
        safe_int<int> resultingChars = ::WideCharToMultiByte(codePage, 0, wideString, wideCount, &narrowString[0], narrowCount, nullptr, nullptr);
		if (!resultingChars || resultingChars != narrowCount)
			throw_error("Cannot convert to narrow string");
		return narrowString;
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
