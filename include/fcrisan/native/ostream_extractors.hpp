#ifndef FCRISAN_NATIVE_OSTREAM_EXTRACTORS_HPP
#define FCRISAN_NATIVE_OSTREAM_EXTRACTORS_HPP

#pragma once

#include <iosfwd>

namespace std {
	std::ostream & operator<<(std::ostream &, const wchar_t *);
}

#endif // FCRISAN_NATIVE_OSTREAM_EXTRACTORS_HPP
