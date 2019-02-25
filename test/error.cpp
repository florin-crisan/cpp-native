#include <fcrisan/native/error.hpp>
#include <cassert>
#include <iostream>

#if defined(__linux__)

int main() {
    using namespace fcrisan::native;
    using namespace std;

    // Test clear_error actually clears the system error code.
    {
        errno = 99;
        clear_error();
        auto e = errno;
        assert(e == 0);
    }

    // Test that last_error returns the last system error code.
    {
        errno = 1;
        auto e = last_error();
        assert(e == 1);
    }

    // Test that last_error returns the last system error code even if it's 0.
    {
        errno = 0;
        auto e = last_error();
        assert(e == 0);
    }

    // Test that last_system_error returns the last system error code.
    {
        errno = 3;
        auto s = last_system_error();
        assert(s.category() == system_category());
        assert(s.value() == 3);
    }

    // Test that last_system_error returns the last system error code even if it's 0.
    {
        errno = 0;
        auto s = last_system_error();
        assert(s.category() == system_category());
        assert(s.value() == 0);
    }

    // throw_error with last system error
    {
        clear_error();
        try {
            errno = 4;
            throw_error("test");
            assert(!"No exception thrown");
        }
        catch (const std::system_error &ex) {
            assert(ex.code().value() == 4);
        }
    }

    // throw_error with explicit system error
    {
        clear_error();
        try {
            errno = 5;
            throw_error("test", 123);
            assert(!"No exception thrown");
        }
        catch (const std::system_error &ex) {
            assert(ex.code().value() == 123);
        }
    }

    // throw_error with explicit error_code
    {
        clear_error();
        try {
            errno = 6;
            throw_error("test", std::error_code{256, std::system_category()});
            assert(!"No exception thrown");
        }
        catch (const std::system_error &ex) {
            assert(ex.code().value() == 256);
        }
    }
}

#endif
