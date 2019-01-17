#ifndef FCRISAN_NATIVE_ERROR_HPP
#define FCRISAN_NATIVE_ERROR_HPP

#pragma once

#include <system_error>

namespace fcrisan::native {

#ifdef _WIN32
	/**
		Native operating system error code type.
		
		Set by operating system native API functions.
	*/
	typedef /*DWORD*/ unsigned long err_code;
#else
#   error Not implemented for this platform.
#endif

	/**
		Clears last operating system error for the thread.
	*/
	void clear_error();
	/**
		Returns the last operating system error for the thread.
	*/
	err_code last_error();
	/**
		Returns the last operating system error for the thread, wrapped in a standard std::error_code.
	*/
	std::error_code last_system_error();
	/**
		Throws a std::system_error for the given native error code.
	*/
	void throw_error(const char *message, err_code = last_error());
	/**
		Throws a std::system_error for the given std::error_code.
	*/
	void throw_error(const char *message, std::error_code);

} // namespace fcrisan::native

#endif // FCRISAN_NATIVE_ERROR_HPP
