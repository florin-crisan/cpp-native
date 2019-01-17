#ifndef FCRISAN_NATIVE_CODE_PAGE_HPP
#define FCRISAN_NATIVE_CODE_PAGE_HPP

#pragma once

namespace fcrisan::native {

#ifdef _WIN32

	typedef unsigned int code_page_id;

	namespace code_pages {
		static const code_page_id utf8 = /*CP_UTF8*/ 65001;
		static const code_page_id utf16 = 1200; // Not actually a system code page, just an ID that we (and .NET) use.
	}
#else
#   error Not implemented for this platform.
#endif

}

#endif // FCRISAN_NATIVE_CODE_PAGE_HPP
