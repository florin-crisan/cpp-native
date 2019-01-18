#include <fcrisan/native/ostream_extractors.hpp>
#include <fcrisan/native/text.hpp>
#include <iostream>
#include <string>

namespace std {

	ostream & operator<<(ostream &stream, const wchar_t *what) {
		using namespace fcrisan;
		auto narrowString = native::to_narrow_string(what);
		return stream << narrowString.c_str();
	}

}
