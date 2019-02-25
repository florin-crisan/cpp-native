#include <fcrisan/native/error.hpp>
#if defined(_WIN32)
#   include <Windows.h>
#   undef min
#   undef max
#elif defined(__linux__)
#   include <cerrno>
#else
#   error Not implemented for this platform.
#endif

namespace fcrisan::native {

#if defined(_WIN32)
    static_assert(sizeof(err_code) == sizeof(DWORD));
#elif defined(__linux__)
    static_assert(sizeof(err_code) == sizeof(int));
#endif

	void clear_error() {
#if defined(_WIN32)
		::SetLastError(0);
#elif defined(__linux__)
        errno = 0;
#endif
	}

	err_code last_error() {
#if defined(_WIN32)
        return ::GetLastError();
#elif defined(__linux__)
        return errno;
#endif
    }

	std::error_code last_system_error() {
        static_assert(sizeof(err_code) == sizeof(int));
        auto lastError = last_error();
		return std::error_code(static_cast<int>(lastError), std::system_category());
	}

    [[noreturn]] void throw_error(const char *message, err_code native_code /*= last_error()*/) {
        static_assert(sizeof(err_code) == sizeof(int));
        auto syserror = std::error_code(static_cast<int>(native_code), std::system_category());
		throw_error(message, syserror);
	}

    [[noreturn]] void throw_error(const char *message, std::error_code code) {
		throw std::system_error(code, message);
	}

} // namespace fcrisan::native
