#ifndef FCRISAN_NATIVE_CODE_PAGE_HPP
#define FCRISAN_NATIVE_CODE_PAGE_HPP

#pragma once

namespace fcrisan::native {

#ifdef _WIN32

	typedef unsigned int code_page_id;

	namespace code_pages {
		const code_page_id ascii = 20127;
		const code_page_id utf8 = 65001;
		const code_page_id utf16_le = 1200; // Not actually a system code page, just an ID that we (and .NET) use.
		const code_page_id utf16_be = 1201; // Not actually a system code page, just an ID that we (and .NET) use.
		const code_page_id dos_us = 437;
		const code_page_id dos_western_europe = 850;
		const code_page_id dos_eastern_europe = 852;
		const code_page_id windows_western_europe = 1252;
		const code_page_id windows_eastern_europe = 1250;

#   if defined(_M_IX86) || defined(_M_X64)
		const code_page_id utf16 = utf16_le;
#   else
#       error Todo: figure out if machine is little endian or big endian.
#   endif
	}
#else
#   error Not implemented for this platform.
#endif

}

#endif // FCRISAN_NATIVE_CODE_PAGE_HPP
