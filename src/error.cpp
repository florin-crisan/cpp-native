#include <fcrisan/native/error.hpp>

#include <Windows.h>

namespace fcrisan::native {

	void clear_error() {
		::SetLastError(0);
	}

	err_code last_error() {
		return ::GetLastError();
	}

	std::error_code last_system_error() {
		static_assert(sizeof(int) == sizeof(DWORD));
		DWORD lastError = ::GetLastError();
		return std::error_code(static_cast<int>(lastError), std::system_category());
	}

	void throw_error(const char *message, err_code native_code /*= last_error()*/) {
		static_assert(sizeof(int) == sizeof(DWORD));
		auto syserror = std::error_code(static_cast<int>(native_code), std::system_category());
		throw_error(message, syserror);
	}

	void throw_error(const char *message, std::error_code code) {
		throw std::system_error(code, message);
	}

} // namespace fcrisan::native
