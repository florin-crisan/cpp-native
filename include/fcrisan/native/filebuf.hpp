#ifndef FCRISAN_NATIVE_FILEBUF_HPP
#define FCRISAN_NATIVE_FILEBUF_HPP

#pragma once

#include <streambuf>
#include <memory>
#include <fcrisan/native/code_page.hpp>

namespace fcrisan::native {

	class text_writer;

	/**
		@brief std::basic_streambuf implementation that writes to an operating system file/console handle in the given code page.

		Rationale

		The normal C++ way of doing things is to use the standard std::basic_streambuf or std::basic_filebuf implementation via an iostream and provide a locale with a std::codecvt<StreamChar, char, std::mbstate_t>.
		This can be less than ideal since the standard implementations:
		- Don't provide access to all operating system features
		- Tying the output encoding to a particular locale is unnatural on Windows
		- Certain implementations (such as the one in Visual Studio) make it hard to work with UTF-8 in memory.
		Instead, it is easier to replace the whole streambuf entirely.

		Features:
		- Can output to any operating system file handle, including console, pipe or file.
		- Can use any of the system code pages -- uses system standard code page conversion functions.
		- Always outputs Unicode to Windows console.
		- Precompose characters (where possible) so they look right on Windows console (which doesn't support precomposed characters).
		- Encoding for file or pipe output defaults to console code page.

		Usage example:

		@code
		cout.set_rdbuf(new fcrisan::native_streambuf<char>);
		// Use cout normally:
		cout << u8"Știință și tehnică\n";
		@endcode
	*/
	template <typename Ch, typename Tr = std::char_traits<Ch>>
	class filebuf : public std::basic_streambuf<Ch, Tr> {
	public:
		filebuf(text_writer &out);
		filebuf(const filebuf &) = delete;
		filebuf(filebuf &&) = delete;
		filebuf & operator=(const filebuf &) = delete;

		virtual ~filebuf() override;

	protected:
		virtual std::streamsize xsputn(const char_type* s, std::streamsize count) override;
		virtual int_type overflow(int_type ch = traits_type::eof()) override;

	private:
		typedef std::basic_streambuf<Ch, Tr> base_t;
		struct impl;
		std::unique_ptr<impl> pimpl;
	};

	extern template class filebuf<char, std::char_traits<char>>;
	extern template class filebuf<wchar_t, std::char_traits<wchar_t>>;

} // namespace fcrisan::native

#endif // FCRISAN_NATIVE_FILEBUF_HPP
