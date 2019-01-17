#ifndef FCRISAN_NATIVE_TEXT_HPP
#define FCRISAN_NATIVE_TEXT_HPP

#pragma once

#include <string_view>
#include <fcrisan/native/code_page.hpp>

namespace fcrisan::native {

	std::wstring to_wide_string(const char *what, std::size_t count, code_page_id cp = code_pages::utf8);

	std::string to_narrow_string(const wchar_t *what, std::size_t count, code_page_id cp = code_pages::utf8);

	std::wstring precompose(const wchar_t *what, std::size_t count);

	void replace_legacy_chars(wchar_t *what, std::size_t count);

	std::wstring normalize_eol(const wchar_t *p, std::size_t count, const wchar_t *eol);

}

#endif // FCRISAN_NATIVE_TEXT_HPP
