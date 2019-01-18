#include <fcrisan/native/filebuf.hpp>
#include <fcrisan/native/text_writer.hpp>
#include <SafeInt.hpp>

template <typename T> using safe_int = SafeInt<T>;

namespace fcrisan::native {

	template <typename Ch, typename Tr>
	struct filebuf<Ch, Tr>::impl {
		text_writer &writer;

		impl(text_writer &w) : writer{ w } {}
	};

	template <typename Ch, typename Tr>
	filebuf<Ch, Tr>::filebuf(text_writer &w)
		: pimpl{ new impl{w} }
	{
	}

	template <typename Ch, typename Tr>
	filebuf<Ch, Tr>::~filebuf() {
	}

	template <typename Ch, typename Tr>
	std::streamsize filebuf<Ch, Tr>::xsputn(const filebuf<Ch, Tr>::char_type * inputString, std::streamsize inputCount) {
		pimpl->writer.write(inputString, safe_int<std::size_t>(inputCount));
		return inputCount;
	}

	template <typename Ch, typename Tr>
	typename filebuf<Ch, Tr>::int_type filebuf<Ch, Tr>::overflow(filebuf<Ch, Tr>::int_type ch) {
		if (ch == traits_type::eof())
			return 0; // Nothing to do since we don't cache.
		auto c = static_cast<char_type>(ch);
		xsputn(&c, 1);
		return 0;
	}

	// Explicit instantiation
	template class filebuf<char, std::char_traits<char>>;
	template class filebuf<wchar_t, std::char_traits<wchar_t>>;

}
