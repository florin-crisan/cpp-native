#include <fcrisan/native/file.hpp>
#include <fcrisan/native/error.hpp>
#include <SafeInt.hpp>

#include <unistd.h>

template <typename T> using safe_int = SafeInt<T>;

namespace fcrisan::native {

    static_assert(sizeof(std::streamsize) == sizeof(::ssize_t));

	struct file::impl {
		bool ownsHandle;
		file_handle h;
	};

	file::~file() {
		try_close();
	}

	file::file(file_handle h, bool ownsHandle)
		: pimpl(new impl{ ownsHandle, h })
	{
        if (h == -1)
			throw std::invalid_argument("bad file handle");
	}

	bool file::try_close() {
		if (!pimpl->ownsHandle)
			return true;
        if (pimpl->h == 0)
			return true;

		auto h = pimpl->h;
        pimpl->h = 0;
		pimpl->ownsHandle = false;

		clear_error();
        return ::close(h);
	}

	void file::close() {
		if (!try_close())
			throw_error("cannot close file handle");
	}

    void file::rewind() {
        clear_error();
        if (::lseek64(pimpl->h, 0, SEEK_SET) == -1)
            throw_error("cannot rewind file");
	}

	std::streamsize file::position() const {
        static_assert(sizeof(::off64_t) == sizeof(std::streamsize));
        SafeInt<::off64_t> pos;
        clear_error();
        pos = ::lseek64(pimpl->h, 0, SEEK_CUR);
        if (pos == -1)
            throw_error("cannot get file position");
        return pos;
	}

	void file::position(std::streamsize pos) {
#if 0
		static_assert(sizeof(std::streamsize) == sizeof(LARGE_INTEGER));
		LARGE_INTEGER newPos;
		newPos.QuadPart = pos;
		LARGE_INTEGER actualPos;
		clear_error();
		if (!::SetFilePointerEx(pimpl->h, newPos, &actualPos, FILE_BEGIN))
			throw_error("cannot set file position");
		if (newPos.QuadPart != actualPos.QuadPart)
			throw_error("cannot set file position");
#endif
	}

	std::streamsize file::size() const {
        return 0;
#if 0
		static_assert(sizeof(std::streamsize) == sizeof(LARGE_INTEGER));
		LARGE_INTEGER fileSize;
		clear_error();
		if (!::GetFileSizeEx(pimpl->h, &fileSize))
			throw_error("cannot set file position");
		return fileSize.QuadPart;
#endif
	}

	file::operator file_handle() {
		return pimpl->h;
	}

	file_handle file::handle() {
		return pimpl->h;
	}

    std::size_t file::read(void *buffer, std::size_t bytesToRead) {
        if (!buffer)
            throw std::invalid_argument("buffer pointer is null");
        clear_error();
        ::ssize_t bytesRead = ::read(pimpl->h, buffer, bytesToRead);
        if (bytesRead == -1)
            throw_error("cannot read from file");
        return safe_int<size_t>(bytesRead);
	}

    void file::write(const void *buffer, std::size_t bytesToWrite) {
        if (!buffer)
            throw std::invalid_argument("buffer pointer is null");
        clear_error();
        safe_int<::ssize_t> bytesWritten = ::write(pimpl->h, buffer, bytesToWrite);
		if (bytesWritten != bytesToWrite)
			throw_error("partial write to file");
	}

	//---
#if 0
	static_assert(sizeof(file_access) == sizeof(DWORD));
	static_assert(sizeof(share_mode) == sizeof(DWORD));
	static_assert(sizeof(create_mode) == sizeof(DWORD));

	std::shared_ptr<file> open_file(const char *fileName, file_access access, share_mode share, create_mode creationMode) {
		auto wideName = to_wide_string(fileName);
		return open_file(wideName.c_str(), access, share, creationMode);
	}

	std::shared_ptr<file> open_file(const wchar_t *fileName, file_access access, share_mode share, create_mode creationMode) {
		clear_error();
		HANDLE h = ::CreateFileW(fileName, static_cast<DWORD>(access), static_cast<DWORD>(share), nullptr, static_cast<DWORD>(creationMode), 0, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			throw_error("cannot open file");
		return std::make_shared<file>(h, true);
	}

	pipe create_anonymous_pipe() {
		HANDLE hWrite;
		HANDLE hRead;

		clear_error();
		if (!::CreatePipe(&hRead, &hWrite, nullptr, 0))
			throw_error("cannot create anonymous pipe");

		return pipe {
			std::make_shared<file>(hRead, true),
			std::make_shared<file>(hWrite, true)
		};
	}

#endif
}
