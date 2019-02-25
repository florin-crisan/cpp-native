#include <fcrisan/native/file.hpp>
#include <fcrisan/native/error.hpp>
#include <cassert>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <typeinfo>
#include <array>
#include <string>
#include <cstring>
#include <algorithm>

#include <SafeInt.hpp>

#include <fcntl.h>
#include <unistd.h>

constexpr const char *tempFileName = "/tmp/79932065-140F-41C4-B9E7-6AAA67254ECF";

void test_close() {
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open("/etc/passwd", O_RDONLY);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    {
        nat::file f{h, true};
    }

    err = errno;
    assert(err == 0);

    err = 0;
    auto close_ret = ::close(h);
    err = errno;
    assert(close_ret == -1);
    assert(err == EBADF);
}

void test_noclose() {
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open("/etc/passwd", O_RDONLY);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    {
        nat::file f{h, false};
    }

    err = errno;
    assert(err == 0);

    errno = 0;
    auto close_ret = ::close(h);
    err = errno;
    assert(close_ret == 0);
    assert(err == 0);
}

void test_read_args() {
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open("/etc/passwd", O_RDONLY);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    nat::file f{h, true};
    try {
        f.read(nullptr, 0);
    }
    catch (const std::invalid_argument &) {
        goto ok;
    }
    catch (...) {
        // continue;
    }
    assert(!"should have thrown a std::invalid_argument");

    ok:
    assert(1);
}

void test_read_zero() {
    using namespace std;
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open("/etc/passwd", O_RDONLY);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    nat::file f{h, true};

    array<byte,1> buffer;
    auto bytesRead = f.read(&buffer[0], 0);
    err = errno;
    assert(bytesRead == 0);
    assert(err == 0);
}

void test_read_noaccess() {
    using namespace std;
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open(tempFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    array<byte, 32> buf;

    nat::file f{h, true};
    try {
        f.read(&buf[0], buf.size());
    }
    catch (const std::system_error &ex) {
        assert(ex.code().value() == EBADF);
        goto ok;
    }
    catch (...) {
        // not ok
    }
    assert(!"should've thrown an exception");

ok:
    assert(true);
}

void test_read() {
    using namespace std;
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open(tempFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    const string firstLine {"This is the 1st line\n"};
    const string secondLine {"This is the 2nd line\n"};
    const string thirdLine {"This is the 3rd line\n"};

    nat::file f{h, true};

    SafeInt<::ssize_t> bytesWritten;

    errno = 0;
    bytesWritten = ::write(f, &firstLine[0], firstLine.size());
    err = errno;
    assert(bytesWritten == firstLine.size());
    assert(err == 0);

    errno = 0;
    bytesWritten = ::write(f, &secondLine[0], secondLine.size());
    err = errno;
    assert(bytesWritten == secondLine.size());
    assert(err == 0);

    errno = 0;
    bytesWritten = ::write(f, &thirdLine[0], thirdLine.size());
    err = errno;
    assert(bytesWritten == thirdLine.size());
    assert(err == 0);

    errno = 0;
    ::off_t seekResult = ::lseek(f, 0, SEEK_SET);
    err = errno;
    assert(seekResult != -1);
    assert(err == 0);

    array<char, 64> buffer;
    std::size_t bytesRead;

    // - first read starts from the beginning
    memset(&buffer[0], 0, buffer.size());
    bytesRead = f.read(&buffer[0], firstLine.size());
    err = nat::last_error();
    assert(err == 0);
    assert(bytesRead == firstLine.size());
    assert(firstLine == &buffer[0]);

    // - second read continues
    memset(&buffer[0], 0, buffer.size());
    bytesRead = f.read(&buffer[0], secondLine.size());
    err = nat::last_error();
    assert(err == 0);
    assert(bytesRead == secondLine.size());
    assert(secondLine == &buffer[0]);

    // - last read doesn't fill the buffer completely
    memset(&buffer[0], 0, buffer.size());
    bytesRead = f.read(&buffer[0], buffer.size());
    err = nat::last_error();
    assert(err == 0);
    assert(bytesRead == thirdLine.size());
    assert(thirdLine == &buffer[0]);

    // - subsequent reads read 0 bytes without failing
    memset(&buffer[0], 0, buffer.size());
    bytesRead = f.read(&buffer[0], buffer.size());
    err = nat::last_error();
    assert(err == 0);
    assert(bytesRead == 0);
    assert(find_if(buffer.begin(), buffer.end(), [](auto c) { return c != 0; }) == buffer.end());
}

void test_write_args() {
    namespace nat = fcrisan::native;

    int err;

    errno = 0;
    int h = ::open(tempFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    nat::file f{h, true};
    try {
        f.write(nullptr, 0);
    }
    catch (const std::invalid_argument &) {
        goto ok;
    }
    catch (...) {
        // continue;
    }
    assert(!"should have thrown a std::invalid_argument");

    ok:
    assert(1);
}

void test_write() {
    namespace nat = fcrisan::native;
    using namespace std;

    int err;

    errno = 0;
    int h = ::open(tempFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    errno = 0;
    off_t pos = ::lseek(h, 0, SEEK_CUR);
    err = errno;
    assert(pos == 0);
    assert(err == 0);

    nat::file f{h, true};

    string buffer{"Test\n"};

    // Writing 0 bytes should do nothing.
    f.write(&buffer[0], 0);
    err = errno;
    assert(err == 0);

    errno = 0;
    pos = ::lseek(h, 0, SEEK_CUR);
    err = errno;
    assert(pos == 0);
    assert(err == 0);

    // Writing should just work...
    f.write(&buffer[0], buffer.size());
    err = errno;
    assert(err == 0);
    f.write(&buffer[0], buffer.size());
    err = errno;
    assert(err == 0);

    // The current position should reflect the number of bytes written.
    errno = 0;
    pos = ::lseek(h, 0, SEEK_CUR);
    err = errno;
    assert(SafeInt<size_t>(pos) == 2 * buffer.size());
    assert(err == 0);

    // Rewind so we can read what was written.
    errno = 0;
    pos = ::lseek(h, 0, SEEK_SET);
    err = errno;
    assert(err == 0);
    assert(pos == 0);

    string fileContents;
    fileContents.resize(64);

    errno = 0;
    SafeInt<ssize_t> bytesRead = ::read(f, &fileContents[0], fileContents.size());
    err = errno;
    assert(bytesRead == 2 * buffer.size());
    assert(errno == 0);
    fileContents.resize(bytesRead);
    assert(fileContents == "Test\nTest\n");
}

void test_write_noaccess() {
    namespace nat = fcrisan::native;
    using namespace std;

    int err;

    // Make sure file exists.
    errno = 0;
    int h = ::open(tempFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    string contents{"Some content"};

    {
        nat::file f{h, true};
        f.write(&contents[0], contents.size());
    }

    // Open for reading; write should fail.
    errno = 0;
    h = ::open(tempFileName, O_RDONLY, S_IRUSR | S_IWUSR);
    err = errno;
    assert(h != -1);
    assert(err == 0);

    try {
        nat::file f{h, true};
        f.write(&contents[0], contents.size());
    }
    catch (const std::system_error &ex) {
        assert(ex.code().value() == EBADF);
        goto ok;
    }
    catch (...) {
        // continue to assert
    }
    assert(!"expected a std::system_error with EBADF");

ok:
    return;
}

void test_position() {
    namespace n = fcrisan::native;
    using namespace std;

    // If we get here, assume reading and writing work.
    int err;
    int h = ::open(tempFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (h == -1)
        n::throw_error("Cannot open file for writing");

    n::file f{h, true};
    assert(f.position() == 0);

    errno = 0;
    SafeInt<__off64_t> pos = lseek64(f, 0, SEEK_CUR);
    err = errno;
    assert(pos == 0);
    assert(err == 0);

    string line1{"Line 1\n"};
    string line2{"Line 2\n"};

    f.write(&line1[0], line1.size());
    assert(f.position() == SafeInt<streamsize>(line1.size()));
    errno = 0;
    pos = lseek64(f, 0, SEEK_CUR);
    err = errno;
    assert(pos == line1.size());
    assert(err == 0);

    f.rewind();
    assert(f.position() == 0);
    errno = 0;
    pos = lseek64(f, 0, SEEK_CUR);
    err = errno;
    assert(pos == 0);
    assert(err == 0);

}

int main() {
    using namespace std;
    try {
        // file handle is closed when owns_handle is set.
        test_close();
        // file handle is not closed when owns_handle is not set.
        test_noclose();

        // read
        // - complains about null buffer pointer
        test_read_args();
        // - zero-length buffer returns 0 bytes
        test_read_zero();

        // - trying to read without permission throws system_error
        test_read_noaccess();

        test_read();

        // write
        // - complains about null buffer pointer
        test_write_args();
        // - zero-length buffer writes 0 bytes
        // - first write writes to beginning of file
        // - next write writes where the last one left off
        test_write();
        // - trying to write without permission throws system_error
        test_write_noaccess();

        // rewind moves back to 0 (if a file)
        // position correctly tells position
        // - 0
        // - nonzero
        // - large number (could be tricky)
        // - end of file
        // position sets position correctly
        // - 0 = beginning of file
        // - nonzero
        // - jump forwards
        // - jump backwards
        // - jump to same position
        // - jump to end of file
        // - jump past end of file
        // size returns file size correctly
        // - 0
        // - nonzero
        // - updated with writes
        // - unaffected by jumps
        test_position();

        // test open_file
        // - bad file name
        // - various access rights
        // - various creation dispositions
        // - various access modes

        // test create_anonymous_pipe

        return 0;
    }
    catch (const exception &ex) {
        cerr << "[" << typeid(ex).name() << "] " << ex.what() << endl;
        return 1;
    }

}
